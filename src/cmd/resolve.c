// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <apg/graph.h>
#include <apg/util.h>

#include "resolve.h"
#include "cmd_common.h"
#include "../cli/ui.h"
#include "../net/api.h"
#include "../net/downloader.h"
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

static const struct package *
pkg_set_find(const struct pkg_set *set, const char *name)
{
    for (size_t i = 0; i < set->count; i++)
        if (strcmp(set->items[i]->meta->name, name) == 0)
            return set->items[i];
    return NULL;
}

bool
pkg_set_contains(const struct pkg_set *set, const char *name)
{
    return pkg_set_find(set, name) != NULL;
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
fetch_from_url(const char *url, const struct tulpar_config *cfg)
{
    char *pkgs_dir = path_join(cfg->cache_dir, "pkgs");
    mkdir_p(pkgs_dir);

    char *filename = url_cache_filename(url);
    char *dest_path = path_join(pkgs_dir, filename);
    free(filename);
    free(pkgs_dir);

    ui_debugf("downloading %s to %s", url, dest_path);
    struct http_response resp = {0};
    ui_progress_start(_("Downloading"), url, 0, UI_PROGRESS_BYTES);
    bool downloaded =
        http_download(url, dest_path, cmd_download_progress, NULL, &resp);
    ui_progress_finish();
    if (!downloaded)
    {
        ui_errorf(_("cannot download %s: %s"), url, http_last_error());
        free(dest_path);
        return NULL;
    }
    cmd_report_downloaded(url, dest_path);

    char sig_url[2048];
    snprintf(sig_url, sizeof(sig_url), "%s.sig", url);
    char sig_path[600];
    snprintf(sig_path, sizeof(sig_path), "%s.sig", dest_path);
    struct http_response sig_resp = {0};
    http_download(sig_url, sig_path, NULL, NULL, &sig_resp);

    ui_debugf("parsing %s", dest_path);
    struct package *pkg = cmd_parse_package(dest_path);
    if (!pkg)
        ui_errorf(_("%s is not a valid package: %s"), url, cmd_libapg_reason());
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
fetch_from_git(const char *arg, const struct tulpar_config *cfg)
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
    struct package *pkg = cmd_parse_package(dest_path);
    if (!pkg)
        ui_errorf(_("%s did not produce a valid package: %s"), arg,
                  cmd_libapg_reason());
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
        bool refresh_failed = false;
        struct repo_index *idx =
            repodata_load(repos->urls[i], cfg->cache_dir, cfg->repodata_ttl,
                          false, &refresh_failed);
        if (refresh_failed)
            cmd_warn_repo_unreachable(repos->urls[i], idx != NULL);
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

static bool
lookup_in_repos(const char *name, ver_op_t op, const char *version,
                const struct repo_list *repos, const struct tulpar_config *cfg,
                struct remote_pick *out)
{
    ui_debugf("resolving %s against %d configured repo(s)", name, repos->count);

    for (int i = 0; i < repos->count; i++)
    {
        const char *base_url = repos->urls[i];

        ui_debugf("querying %s/api/v2/packages/%s", base_url, name);
        struct repo_index *idx = api_get_package(base_url, name);
        if (!idx)
        {
            if (http_last_not_found())
                ui_debugf("%s does not have %s", base_url, name);
            else
                cmd_warn_repo_unreachable(base_url, false);
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

        const char *channel = best->channel[0] ? best->channel : "stable";
        const char *arch =
            best->architecture[0] ? best->architecture : "noarch";
        ui_debugf("selected %s %s (%s/%s) from %s", best->name, best->version,
                  channel, arch, base_url);

        char *pkgs_dir = path_join(cfg->cache_dir, "pkgs");
        mkdir_p(pkgs_dir);
        char filename[512];
        snprintf(filename, sizeof(filename), "%s-%s-%s.apg", best->name,
                 best->version, arch);
        char *dest_path = path_join(pkgs_dir, filename);
        free(pkgs_dir);

        memset(out, 0, sizeof(*out));
        snprintf(out->name, sizeof(out->name), "%s", best->name);
        snprintf(out->version, sizeof(out->version), "%s", best->version);
        snprintf(out->base_url, sizeof(out->base_url), "%s", base_url);
        snprintf(out->label, sizeof(out->label), "%s %s", best->name,
                 best->version);
        api_download_url(out->url, sizeof(out->url), base_url, channel,
                         best->name, best->version, arch);
        snprintf(out->sig_url, sizeof(out->sig_url), "%s.sig", out->url);
        snprintf(out->dest_path, sizeof(out->dest_path), "%s",
                 dest_path ? dest_path : "");
        snprintf(out->sig_path, sizeof(out->sig_path), "%s.sig",
                 out->dest_path);
        free(dest_path);
        repo_index_free(idx);
        return true;
    }

    return false;
}

static bool
lookup_or_provider(const char *name, ver_op_t op, const char *version,
                   const struct repo_list *repos,
                   const struct tulpar_config *cfg,
                   const struct provider_pref *prefs, size_t pref_count,
                   bool assume_yes, struct remote_pick *out)
{
    if (lookup_in_repos(name, op, version, repos, cfg, out))
        return true;

    char provider_name[256];
    if (!resolve_choose_provider(name, repos, cfg, prefs, pref_count,
                                 assume_yes, provider_name,
                                 sizeof(provider_name)))
        return false;

    ui_debugf("%s resolved via provides to package %s", name, provider_name);
    return lookup_in_repos(provider_name, VER_OP_ANY, NULL, repos, cfg, out);
}

bool
resolve_download(const struct remote_pick *picks, size_t count,
                 const struct tulpar_config *cfg, struct package **out)
{
    struct download_task *tasks = calloc(count * 2, sizeof(*tasks));
    if (!tasks)
        return false;

    for (size_t i = 0; i < count; i++)
    {
        tasks[i * 2].url = picks[i].url;
        tasks[i * 2].dest_path = picks[i].dest_path;
        tasks[i * 2].label = picks[i].label;
        tasks[i * 2 + 1].url = picks[i].sig_url;
        tasks[i * 2 + 1].dest_path = picks[i].sig_path;
        tasks[i * 2 + 1].optional = true;
    }

    download_all_parallel(tasks, count * 2, cfg->max_parallel_downloads);

    bool ok = true;
    for (size_t i = 0; i < count; i++)
    {
        out[i] = NULL;
        if (!tasks[i * 2].ok)
        {
            ui_errorf(_("cannot download %s %s from %s: %s"), picks[i].name,
                      picks[i].version, picks[i].base_url, tasks[i * 2].error);
            ok = false;
            continue;
        }

        ui_debugf("parsing %s", picks[i].dest_path);
        out[i] = cmd_parse_package(picks[i].dest_path);
        if (!out[i])
        {
            ui_errorf(_("%s %s from %s is not a valid package: %s"),
                      picks[i].name, picks[i].version, picks[i].base_url,
                      cmd_libapg_reason());
            ok = false;
        }
    }

    free(tasks);
    if (!ok)
    {
        for (size_t i = 0; i < count; i++)
        {
            package_free(out[i]);
            out[i] = NULL;
        }
    }
    return ok;
}

bool
resolve_lookup_named(const char *name, ver_op_t op, const char *version,
                     const struct repo_list *repos,
                     const struct tulpar_config *cfg, struct remote_pick *out)
{
    return lookup_in_repos(name, op, version, repos, cfg, out);
}

struct package *
resolve_fetch_named(const char *name, ver_op_t op, const char *version,
                    const struct repo_list *repos,
                    const struct tulpar_config *cfg, const char *root_path,
                    bool *failed)
{
    *failed = false;
    struct remote_pick pick;
    if (!lookup_in_repos(name, op, version, repos, cfg, &pick))
        return NULL;

    struct package *pkg = NULL;
    if (!resolve_download(&pick, 1, cfg, &pkg))
        *failed = true;
    return pkg;
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

bool
resolve_dep_installed(struct db_handle *db, const struct dep_constraint *dep)
{
    struct package *installed = db_get(db, dep->name);
    if (!installed)
        installed = db_find_provider(db, dep->name);
    if (!installed)
        return false;

    bool satisfied =
        dep->op == VER_OP_ANY ||
        ver_satisfies(installed->meta->version, dep->op, dep->version);
    package_free(installed);
    return satisfied;
}

static void
warn_skipped_dependencies(struct db_handle *db, const struct pkg_set *set)
{
    for (size_t i = 0; i < set->count; i++)
    {
        const struct package_metadata *meta = set->items[i]->meta;
        for (int j = 0; j < meta->dependencies.count; j++)
        {
            const struct dep_constraint *dep = &meta->dependencies.items[j];
            if (pkg_set_satisfies(set, dep->name) ||
                resolve_dep_installed(db, dep))
                continue;

            char *want = dep_constraint_to_str(dep);
            ui_warnf(_("skipping installation of %s (--nodeps)"),
                     want ? want : dep->name);
            ui_note(_("%s depends on %s and may not work without it"),
                    meta->name, want ? want : dep->name);
            free(want);
        }
    }
}

static bool
pick_listed(const struct remote_pick *picks, size_t count, const char *name)
{
    for (size_t i = 0; i < count; i++)
        if (strcmp(picks[i].name, name) == 0)
            return true;
    return false;
}

static bool
resolve_dependencies(size_t first, struct db_handle *db,
                     const struct repo_list *repos,
                     const struct tulpar_config *cfg, const char *root_path,
                     const struct provider_pref *prefs, size_t pref_count,
                     bool assume_yes, struct pkg_set *out)
{
    while (first < out->count)
    {
        size_t level_end = out->count;
        struct remote_pick *picks = NULL;
        size_t pick_count = 0;
        bool ok = true;

        for (size_t p = first; ok && p < level_end; p++)
        {
            const struct package_metadata *meta = out->items[p]->meta;
            for (int j = 0; ok && j < meta->dependencies.count; j++)
            {
                const struct dep_constraint *dep = &meta->dependencies.items[j];
                if (pkg_set_satisfies(out, dep->name) ||
                    pick_listed(picks, pick_count, dep->name) ||
                    resolve_dep_installed(db, dep))
                    continue;

                struct remote_pick pick;
                if (!lookup_or_provider(dep->name, dep->op, dep->version, repos,
                                        cfg, prefs, pref_count, assume_yes,
                                        &pick))
                {
                    char *want = dep_constraint_to_str(dep);
                    ui_errorf(_("%s requires %s, which was not found in any "
                                "configured repository"),
                              meta->name, want ? want : dep->name);
                    free(want);
                    if (repos->count == 0)
                        ui_hint(_("no repositories are configured; add one "
                                  "with 'tulpar repo add <url>'"));
                    ok = false;
                    break;
                }
                if (pkg_set_contains(out, pick.name) ||
                    pick_listed(picks, pick_count, pick.name))
                    continue;

                struct remote_pick *grown =
                    realloc(picks, (pick_count + 1) * sizeof(*picks));
                if (!grown)
                {
                    ok = false;
                    break;
                }
                picks = grown;
                picks[pick_count++] = pick;
            }
        }

        struct package **fetched =
            ok && pick_count > 0 ? calloc(pick_count, sizeof(*fetched)) : NULL;
        if (ok && pick_count > 0)
            ok = fetched && resolve_download(picks, pick_count, cfg, fetched);
        for (size_t i = 0; ok && i < pick_count; i++)
        {
            if (!pkg_set_add(out, fetched[i]))
            {
                for (size_t k = i; k < pick_count; k++)
                    package_free(fetched[k]);
                ok = false;
            }
        }
        free(fetched);
        free(picks);

        if (!ok)
            return false;
        first = level_end;
    }
    return true;
}

static bool
installed_is_current(struct db_handle *db, const struct package *pkg)
{
    struct package *installed = db_get(db, pkg->meta->name);
    if (!installed)
        return false;

    int cmp = ver_compare(pkg->meta->version, installed->meta->version);
    if (cmp == 0)
        ui_infof(_("%s %s is already installed"), pkg->meta->name,
                 pkg->meta->version);
    else if (cmp < 0)
        ui_warnf(_("skipping %s %s: the installed version %s is newer"),
                 pkg->meta->name, pkg->meta->version, installed->meta->version);
    else
        ui_debugf("%s is installed at %s, upgrading to %s", pkg->meta->name,
                  installed->meta->version, pkg->meta->version);

    package_free(installed);
    return cmp <= 0;
}

static void
note_requested_installed(struct str_list *list, struct db_handle *db,
                         const char *name)
{
    struct package *installed = db_get(db, name);
    if (!installed)
        return;
    package_free(installed);

    for (int i = 0; i < list->count; i++)
        if (strcmp(list->items[i], name) == 0)
            return;

    char **items =
        realloc(list->items, (size_t)(list->count + 1) * sizeof(*items));
    if (!items)
        return;
    list->items = items;
    list->items[list->count] = strdup(name);
    if (list->items[list->count])
        list->count++;
}

bool
resolve_install_closure(char *const *requested, size_t requested_count,
                        struct db_handle *db, const struct repo_list *repos,
                        const struct tulpar_config *cfg, const char *root_path,
                        const struct provider_pref *prefs, size_t pref_count,
                        bool assume_yes, bool nodeps, struct pkg_set *out,
                        struct str_list *requested_installed)
{
    const struct package_metadata **roots =
        calloc(requested_count, sizeof(*roots));
    if (!roots)
        return false;
    size_t root_count = 0;

    for (size_t i = 0; i < requested_count; i++)
    {
        const char *arg = requested[i];
        struct package *pkg = NULL;
        struct package *installed = NULL;

        if (resolve_arg_is_git_url(arg))
        {
            ui_debugf("%s is a git repository URL, cloning", arg);
            pkg = fetch_from_git(arg, cfg);
            if (!pkg)
            {
                ui_errorf(_("failed to install from %s"), arg);
                free(roots);
                return false;
            }
        }
        else if (resolve_arg_is_url(arg))
        {
            ui_debugf("%s is a URL, downloading directly", arg);
            pkg = fetch_from_url(arg, cfg);
            if (!pkg)
            {
                ui_errorf(_("failed to install from %s"), arg);
                free(roots);
                return false;
            }
        }
        else if (ends_with(arg, ".apg") && file_exists(arg))
        {
            ui_debugf("%s is a local archive, parsing directly", arg);
            pkg = cmd_parse_package(arg);
            if (!pkg)
            {
                ui_errorf(_("failed to read package archive %s: %s"), arg,
                          cmd_libapg_reason());
                free(roots);
                return false;
            }
        }
        else if ((installed = db_get(db, arg)) != NULL)
        {
            bool failed = false;
            pkg = resolve_fetch_named(arg, VER_OP_GT, installed->meta->version,
                                      repos, cfg, root_path, &failed);
            if (!pkg && !failed)
                ui_infof(_("%s %s is already installed and up to date"),
                         installed->meta->name, installed->meta->version);
            note_requested_installed(requested_installed, db,
                                     installed->meta->name);
            package_free(installed);
            if (failed)
            {
                free(roots);
                return false;
            }
            if (!pkg)
                continue;
        }
        else
        {
            ui_debugf("%s is not a local archive, resolving by name", arg);
            struct remote_pick pick;
            if (!lookup_or_provider(arg, VER_OP_ANY, NULL, repos, cfg, prefs,
                                    pref_count, assume_yes, &pick))
            {
                ui_errorf(_("package %s not found in any configured repo"),
                          arg);
                if (repos->count == 0)
                    ui_hint(_("no repositories are configured; add one with "
                              "'tulpar repo add <url>'"));
                free(roots);
                return false;
            }
            if (!resolve_download(&pick, 1, cfg, &pkg))
            {
                free(roots);
                return false;
            }
        }

        ui_debugf("resolved %s to %s %s", arg, pkg->meta->name,
                  pkg->meta->version);

        const struct package *queued = pkg_set_find(out, pkg->meta->name);
        if (queued)
        {
            roots[root_count++] = queued->meta;
            package_free(pkg);
            continue;
        }

        note_requested_installed(requested_installed, db, pkg->meta->name);
        if (installed_is_current(db, pkg))
        {
            package_free(pkg);
            continue;
        }

        roots[root_count++] = pkg->meta;

        if (!pkg_set_add(out, pkg))
        {
            package_free(pkg);
            free(roots);
            return false;
        }
    }

    out->explicit_count = out->count;

    if (!nodeps && !resolve_dependencies(0, db, repos, cfg, root_path, prefs,
                                         pref_count, assume_yes, out))
    {
        free(roots);
        return false;
    }

    if (nodeps)
    {
        warn_skipped_dependencies(db, out);
        free(roots);
        return true;
    }

    if (out->count > 0 && root_count > 0)
    {
        int installed_count = 0;
        struct package **installed = db_list(db, &installed_count);

        size_t total_candidates = out->count + (size_t)installed_count;
        const struct package_metadata **candidates =
            malloc(total_candidates * sizeof(*candidates));
        if (!candidates)
        {
            for (int i = 0; i < installed_count; i++)
                package_free(installed[i]);
            free(installed);
            free(roots);
            return false;
        }

        for (size_t i = 0; i < out->count; i++)
            candidates[i] = out->items[i]->meta;

        for (int i = 0; i < installed_count; i++)
            candidates[out->count + (size_t)i] = installed[i]->meta;

        struct package_metadata **selected = NULL;
        size_t selected_count = 0;
        char *conflict = NULL;

        sat_solve_result_t sat_res = dep_graph_resolve_sat(
            candidates, total_candidates, roots, root_count, 10000, 4,
            &selected, &selected_count, &conflict);

        free(candidates);
        for (int i = 0; i < installed_count; i++)
            package_free(installed[i]);
        free(installed);
        free(roots);

        if (sat_res == SAT_SOLVE_UNSATISFIABLE)
        {
            if (conflict)
            {
                ui_errorf(_("dependency conflict: %s"), conflict);
                free(conflict);
            }
            else
            {
                ui_error(_("dependency conflict detected"));
            }
            return false;
        }

        if (sat_res == SAT_SOLVE_SATISFIABLE && selected)
        {
            struct package **filtered = malloc(out->count * sizeof(*filtered));
            if (filtered)
            {
                size_t filtered_count = 0;
                for (size_t i = 0; i < out->count; i++)
                {
                    bool keep = false;
                    for (size_t j = 0; j < selected_count; j++)
                    {
                        if (selected[j] == out->items[i]->meta)
                        {
                            keep = true;
                            break;
                        }
                    }
                    if (keep)
                        filtered[filtered_count++] = out->items[i];
                    else
                        package_free(out->items[i]);
                }
                free(out->items);
                out->items = filtered;
                out->count = filtered_count;
                out->cap = filtered_count;
            }
            free(selected);
        }
        else
        {
            free(selected);
            free(conflict);
        }
    }
    else
    {
        free(roots);
    }

    return true;
}
