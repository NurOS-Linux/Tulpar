// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "resolve.h"
#include "../cli/ui.h"
#include "../net/api.h"
#include "../net/http.h"
#include "../repo/repodata.h"
#include "../util/paths.h"
#include "../util/proc.h"
#include "../i18n.h"

static bool
metadata_satisfies_name(const struct package_metadata *meta, const char *name)
{
    if (strcmp(meta->name, name) == 0)
        return true;

    for (int i = 0; i < meta->provides.count; i++)
        if (strcmp(meta->provides.items[i], name) == 0)
            return true;

    for (int i = 0; i < meta->replaces.count; i++)
        if (strcmp(meta->replaces.items[i], name) == 0)
            return true;

    return false;
}

bool
pkg_set_contains(const struct pkg_set *set, const char *name)
{
    for (size_t i = 0; i < set->count; i++)
        if (strcmp(set->items[i]->meta->name, name) == 0)
            return true;
    return false;
}

static bool
pkg_set_satisfies(const struct pkg_set *set, const char *name)
{
    for (size_t i = 0; i < set->count; i++)
        if (metadata_satisfies_name(set->items[i]->meta, name))
            return true;
    return false;
}

bool
pkg_set_add(struct pkg_set *set, struct package *pkg)
{
    if (set->count == set->cap)
    {
        size_t new_cap = set->cap == 0 ? 8 : set->cap * 2;
        struct package **tmp =
            realloc(set->items, new_cap * sizeof(*set->items));
        if (!tmp)
            return false;
        set->items = tmp;
        set->cap = new_cap;
    }

    set->items[set->count++] = pkg;
    return true;
}

void
pkg_set_free(struct pkg_set *set)
{
    for (size_t i = 0; i < set->count; i++)
        package_free(set->items[i]);
    free(set->items);
    set->items = NULL;
    set->count = 0;
    set->cap = 0;
}

static bool
ends_with(const char *s, const char *suffix)
{
    size_t slen = strlen(s);
    size_t suflen = strlen(suffix);
    return slen >= suflen && strcmp(s + slen - suflen, suffix) == 0;
}

static bool
starts_with(const char *s, const char *prefix)
{
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static bool
file_exists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    fclose(f);
    return true;
}

static bool
dir_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

bool
resolve_arg_is_url(const char *arg)
{
    return starts_with(arg, "http://") || starts_with(arg, "https://") ||
           starts_with(arg, "ftp://");
}

bool
resolve_arg_is_git_url(const char *arg)
{
    if (starts_with(arg, "git+http://") || starts_with(arg, "git+https://") ||
        starts_with(arg, "git+ssh://") || starts_with(arg, "git+git://") ||
        starts_with(arg, "git+file://") || starts_with(arg, "git://"))
        return true;

    const char *hash = strchr(arg, '#');
    size_t len = hash ? (size_t)(hash - arg) : strlen(arg);
    return len >= 4 && strncmp(arg + len - 4, ".git", 4) == 0;
}

static unsigned int
fnv1a_hash(const char *s)
{
    unsigned int hash = 2166136261u;
    while (*s)
    {
        hash ^= (unsigned char)*s++;
        hash *= 16777619u;
    }
    return hash;
}

static char *
url_cache_filename(const char *url)
{
    const char *slash = strrchr(url, '/');
    const char *base = slash ? slash + 1 : url;
    if (base[0] != '\0' && ends_with(base, ".apg"))
        return strdup(base);

    char name[32];
    snprintf(name, sizeof(name), "%08x.apg", fnv1a_hash(url));
    return strdup(name);
}

static struct package *
fetch_from_url(const char *url, const struct tulpar_config *cfg,
               const char *root_path)
{
    char *pkgs_dir = path_join(cfg->cache_dir, "pkgs");
    mkdir_p(pkgs_dir);

    char *filename = url_cache_filename(url);
    char *dest_path = path_join(pkgs_dir, filename);
    free(filename);
    free(pkgs_dir);

    ui_debugf("downloading %s to %s", url, dest_path);
    struct http_response resp = {0};
    if (!http_download(url, dest_path, NULL, NULL, &resp))
    {
        ui_errorf(_("failed to download %s"), url);
        free(dest_path);
        return NULL;
    }

    char sig_url[2048];
    snprintf(sig_url, sizeof(sig_url), "%s.sig", url);
    char sig_path[600];
    snprintf(sig_path, sizeof(sig_path), "%s.sig", dest_path);
    struct http_response sig_resp = {0};
    http_download(sig_url, sig_path, NULL, NULL, &sig_resp);

    ui_debugf("parsing %s", dest_path);
    struct package *pkg = parse_package(dest_path, root_path);
    free(dest_path);
    return pkg;
}

static void
git_split_url(const char *arg, char **clone_url, char **ref)
{
    const char *url = arg;
    if (starts_with(url, "git+"))
        url += 4;

    const char *hash = strchr(url, '#');
    if (hash)
    {
        *clone_url = strndup(url, (size_t)(hash - url));
        *ref = strdup(hash + 1);
    }
    else
    {
        *clone_url = strdup(url);
        *ref = NULL;
    }
}

static struct package *
fetch_from_git(const char *arg, const struct tulpar_config *cfg,
               const char *root_path)
{
    char *clone_url = NULL;
    char *ref = NULL;
    git_split_url(arg, &clone_url, &ref);

    char *tmp_base = path_join(cfg->cache_dir, "git-tmp");
    mkdir_p(tmp_base);

    char tmpdir[4096];
    snprintf(tmpdir, sizeof(tmpdir), "%s/repo-XXXXXX", tmp_base);
    free(tmp_base);

    if (!mkdtemp(tmpdir))
    {
        ui_errorf(_("failed to create a temporary directory for %s"), arg);
        free(clone_url);
        free(ref);
        return NULL;
    }

    ui_debugf("cloning %s to %s%s%s", clone_url, tmpdir, ref ? " at ref " : "",
              ref ? ref : "");

    bool cloned;
    if (ref)
    {
        char *clone_argv[] = {"git",     "clone", "--quiet",
                              clone_url, tmpdir,  NULL};
        cloned = run_command(clone_argv);
        if (cloned)
        {
            char *checkout_argv[] = {"git",     "-C", tmpdir, "checkout",
                                     "--quiet", ref,  NULL};
            cloned = run_command(checkout_argv);
        }
    }
    else
    {
        char *clone_argv[] = {"git", "clone",   "--quiet", "--depth",
                              "1",   clone_url, tmpdir,    NULL};
        cloned = run_command(clone_argv);
    }

    free(clone_url);
    free(ref);

    if (!cloned)
    {
        ui_errorf(_("failed to clone %s"), arg);
        remove_dir_recursive(tmpdir);
        return NULL;
    }

    char *metadata_path = path_join(tmpdir, "metadata.json");
    bool has_metadata = file_exists(metadata_path);
    free(metadata_path);
    if (!has_metadata)
    {
        ui_errorf(_("%s does not contain a metadata.json at its root"), arg);
        remove_dir_recursive(tmpdir);
        return NULL;
    }

    char *data_path = path_join(tmpdir, "data");
    bool has_data = dir_exists(data_path);
    free(data_path);

    char *scripts_path = path_join(tmpdir, "scripts");
    bool has_scripts = dir_exists(scripts_path);
    free(scripts_path);

    char *pkgs_dir = path_join(cfg->cache_dir, "pkgs");
    mkdir_p(pkgs_dir);

    char name[32];
    snprintf(name, sizeof(name), "git-%08x.apg", fnv1a_hash(arg));
    char *dest_path = path_join(pkgs_dir, name);
    free(pkgs_dir);

    char *tar_argv[8];
    int n = 0;
    tar_argv[n++] = "tar";
    tar_argv[n++] = "-cf";
    tar_argv[n++] = dest_path;
    tar_argv[n++] = "-C";
    tar_argv[n++] = tmpdir;
    tar_argv[n++] = "metadata.json";
    if (has_data)
        tar_argv[n++] = "data";
    if (has_scripts)
        tar_argv[n++] = "scripts";
    tar_argv[n] = NULL;

    ui_debugf("archiving %s into %s", tmpdir, dest_path);
    bool archived = run_command(tar_argv);
    remove_dir_recursive(tmpdir);

    if (!archived)
    {
        ui_errorf(_("failed to archive the cloned repository at %s"), arg);
        free(dest_path);
        return NULL;
    }

    ui_debugf("parsing %s", dest_path);
    struct package *pkg = parse_package(dest_path, root_path);
    free(dest_path);
    return pkg;
}

static bool
resolve_choose_provider(const char *virtual_name, const struct repo_list *repos,
                        const struct tulpar_config *cfg,
                        const struct provider_pref *prefs, size_t pref_count,
                        bool assume_yes, char *out_name, size_t out_size)
{
    for (size_t i = 0; i < pref_count; i++)
    {
        if (strcmp(prefs[i].name, virtual_name) == 0)
        {
            snprintf(out_name, out_size, "%s", prefs[i].pkg_name);
            return true;
        }
    }

    char candidates[16][256];
    int candidate_count = 0;

    for (int i = 0; i < repos->count && candidate_count < 16; i++)
    {
        struct repo_index *idx = repodata_load(repos->urls[i], cfg->cache_dir,
                                               cfg->repodata_ttl, false);
        if (!idx)
            continue;

        for (size_t j = 0; j < idx->count && candidate_count < 16; j++)
        {
            const struct repo_package *cand = &idx->items[j];
            bool matches = false;
            for (size_t k = 0; k < cand->provides_count && !matches; k++)
                if (strcmp(cand->provides[k], virtual_name) == 0)
                    matches = true;
            for (size_t k = 0; k < cand->replaces_count && !matches; k++)
                if (strcmp(cand->replaces[k], virtual_name) == 0)
                    matches = true;
            if (!matches)
                continue;

            bool already_listed = false;
            for (int m = 0; m < candidate_count; m++)
                if (strcmp(candidates[m], cand->name) == 0)
                {
                    already_listed = true;
                    break;
                }
            if (already_listed)
                continue;

            snprintf(candidates[candidate_count], sizeof(candidates[0]), "%s",
                     cand->name);
            candidate_count++;
        }

        repo_index_free(idx);
    }

    if (candidate_count == 0)
        return false;

    if (candidate_count == 1)
    {
        snprintf(out_name, out_size, "%s", candidates[0]);
        return true;
    }

    const char *options[16];
    for (int i = 0; i < candidate_count; i++)
        options[i] = candidates[i];

    char prompt[300];
    snprintf(prompt, sizeof(prompt),
             "multiple packages provide '%s'; which one should be installed?",
             virtual_name);

    int choice = ui_select(prompt, options, candidate_count, assume_yes);
    if (choice < 0)
    {
        ui_errorf(
            _("'%s' is provided by %d packages; use --provider %s=<package> "
              "to pick one"),
            virtual_name, candidate_count, virtual_name);
        return false;
    }

    snprintf(out_name, out_size, "%s", candidates[choice]);
    return true;
}

struct package *
resolve_fetch_by_name(const char *name, ver_op_t op, const char *version,
                      const struct repo_list *repos,
                      const struct tulpar_config *cfg, const char *root_path,
                      const struct provider_pref *prefs, size_t pref_count,
                      bool assume_yes)
{
    ui_debugf("resolving %s against %d configured repo(s)", name, repos->count);

    for (int i = 0; i < repos->count; i++)
    {
        const char *base_url = repos->urls[i];

        ui_debugf("querying %s/api/v2/packages/%s", base_url, name);
        struct repo_index *idx = api_get_package(base_url, name);
        if (!idx)
        {
            ui_debugf("%s did not answer for %s", base_url, name);
            continue;
        }

        const struct repo_package *best = NULL;
        for (size_t j = 0; j < idx->count; j++)
        {
            const struct repo_package *cand = &idx->items[j];
            if (op == VER_OP_ANY || ver_satisfies(cand->version, op, version))
            {
                best = cand;
                break;
            }
        }

        if (!best)
        {
            ui_debugf("%s has no build of %s satisfying the constraint",
                      base_url, name);
            repo_index_free(idx);
            continue;
        }

        const char *use_channel = best->channel[0] ? best->channel : "stable";
        const char *use_arch =
            best->architecture[0] ? best->architecture : "noarch";

        ui_debugf("selected %s %s (%s/%s) from %s", best->name, best->version,
                  use_channel, use_arch, base_url);

        char *pkgs_dir = path_join(cfg->cache_dir, "pkgs");
        mkdir_p(pkgs_dir);

        char filename[512];
        snprintf(filename, sizeof(filename), "%s-%s-%s.apg", best->name,
                 best->version, use_arch);
        char *dest_path = path_join(pkgs_dir, filename);
        free(pkgs_dir);

        ui_debugf("downloading to %s", dest_path);
        bool downloaded =
            api_download(base_url, use_channel, best->name, best->version,
                         use_arch, dest_path, NULL, NULL);
        if (!downloaded)
        {
            ui_debugf("download from %s failed", base_url);
            free(dest_path);
            repo_index_free(idx);
            continue;
        }

        char sig_path[600];
        snprintf(sig_path, sizeof(sig_path), "%s.sig", dest_path);
        api_download_sig(base_url, use_channel, best->name, best->version,
                         use_arch, sig_path);

        ui_debugf("parsing %s", dest_path);
        struct package *pkg = parse_package(dest_path, root_path);
        free(dest_path);
        repo_index_free(idx);

        if (pkg)
            return pkg;

        ui_debugf("failed to parse the downloaded archive for %s", name);
    }

    char provider_name[256];
    if (!resolve_choose_provider(name, repos, cfg, prefs, pref_count,
                                 assume_yes, provider_name,
                                 sizeof(provider_name)))
        return NULL;

    ui_debugf("%s resolved via provides to package %s", name, provider_name);
    return resolve_fetch_by_name(provider_name, VER_OP_ANY, NULL, repos, cfg,
                                 root_path, prefs, pref_count, assume_yes);
}

static struct package *
db_find_provider(struct db_handle *db, const char *name)
{
    int count = 0;
    struct package **all = db_list(db, &count);
    struct package *found = NULL;

    for (int i = 0; i < count; i++)
    {
        if (!found && metadata_satisfies_name(all[i]->meta, name))
            found = all[i];
        else
            package_free(all[i]);
    }
    free(all);
    return found;
}

static bool
resolve_dependency(const struct dep_constraint *dep, struct db_handle *db,
                   const struct repo_list *repos,
                   const struct tulpar_config *cfg, const char *root_path,
                   const struct provider_pref *prefs, size_t pref_count,
                   bool assume_yes, struct pkg_set *out)
{
    if (pkg_set_satisfies(out, dep->name))
    {
        ui_debugf("dependency %s already queued", dep->name);
        return true;
    }

    struct package *installed = db_get(db, dep->name);
    if (!installed)
        installed = db_find_provider(db, dep->name);
    if (installed)
    {
        bool satisfied =
            dep->op == VER_OP_ANY ||
            ver_satisfies(installed->meta->version, dep->op, dep->version);
        package_free(installed);
        if (satisfied)
        {
            ui_debugf("dependency %s already installed and satisfied",
                      dep->name);
            return true;
        }
    }

    ui_debugf("dependency %s needs to be fetched", dep->name);
    struct package *fetched =
        resolve_fetch_by_name(dep->name, dep->op, dep->version, repos, cfg,
                              root_path, prefs, pref_count, assume_yes);
    if (!fetched)
    {
        ui_errorf(_("could not resolve dependency %s in any configured repo"),
                  dep->name);
        return false;
    }

    if (!pkg_set_add(out, fetched))
    {
        package_free(fetched);
        return false;
    }

    for (int i = 0; i < fetched->meta->dependencies.count; i++)
    {
        if (!resolve_dependency(&fetched->meta->dependencies.items[i], db,
                                repos, cfg, root_path, prefs, pref_count,
                                assume_yes, out))
            return false;
    }

    return true;
}

bool
resolve_install_closure(char *const *requested, size_t requested_count,
                        struct db_handle *db, const struct repo_list *repos,
                        const struct tulpar_config *cfg, const char *root_path,
                        const struct provider_pref *prefs, size_t pref_count,
                        bool assume_yes, struct pkg_set *out)
{
    for (size_t i = 0; i < requested_count; i++)
    {
        const char *arg = requested[i];
        struct package *pkg = NULL;

        if (resolve_arg_is_git_url(arg))
        {
            ui_debugf("%s is a git repository URL, cloning", arg);
            pkg = fetch_from_git(arg, cfg, root_path);
            if (!pkg)
            {
                ui_errorf(_("failed to install from %s"), arg);
                return false;
            }
        }
        else if (resolve_arg_is_url(arg))
        {
            ui_debugf("%s is a URL, downloading directly", arg);
            pkg = fetch_from_url(arg, cfg, root_path);
            if (!pkg)
            {
                ui_errorf(_("failed to install from %s"), arg);
                return false;
            }
        }
        else if (ends_with(arg, ".apg") && file_exists(arg))
        {
            ui_debugf("%s is a local archive, parsing directly", arg);
            pkg = parse_package(arg, root_path);
            if (!pkg)
            {
                ui_errorf(_("failed to read package archive %s"), arg);
                return false;
            }
        }
        else
        {
            ui_debugf("%s is not a local archive, resolving by name", arg);
            pkg =
                resolve_fetch_by_name(arg, VER_OP_ANY, NULL, repos, cfg,
                                      root_path, prefs, pref_count, assume_yes);
            if (!pkg)
            {
                ui_errorf(_("package %s not found in any configured repo"),
                          arg);
                return false;
            }
        }

        ui_debugf("resolved %s to %s %s", arg, pkg->meta->name,
                  pkg->meta->version);

        if (pkg_set_contains(out, pkg->meta->name))
        {
            package_free(pkg);
            continue;
        }

        if (!pkg_set_add(out, pkg))
        {
            package_free(pkg);
            return false;
        }

        for (int j = 0; j < pkg->meta->dependencies.count; j++)
        {
            if (!resolve_dependency(&pkg->meta->dependencies.items[j], db,
                                    repos, cfg, root_path, prefs, pref_count,
                                    assume_yes, out))
                return false;
        }
    }

    return true;
}
