// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resolve.h"
#include "../cli/ui.h"
#include "../net/api.h"
#include "../util/paths.h"

bool
pkg_set_contains(const struct pkg_set *set, const char *name)
{
    for (size_t i = 0; i < set->count; i++)
        if (strcmp(set->items[i]->meta->name, name) == 0)
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
file_exists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    fclose(f);
    return true;
}

struct package *
resolve_fetch_by_name(const char *name, ver_op_t op, const char *version,
                      const struct repo_list *repos,
                      const struct tulpar_config *cfg, const char *root_path)
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

    return NULL;
}

static bool
resolve_dependency(const struct dep_constraint *dep, struct db_handle *db,
                   const struct repo_list *repos,
                   const struct tulpar_config *cfg, const char *root_path,
                   struct pkg_set *out)
{
    if (pkg_set_contains(out, dep->name))
    {
        ui_debugf("dependency %s already queued", dep->name);
        return true;
    }

    struct package *installed = db_get(db, dep->name);
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
    struct package *fetched = resolve_fetch_by_name(
        dep->name, dep->op, dep->version, repos, cfg, root_path);
    if (!fetched)
    {
        ui_errorf("could not resolve dependency %s in any configured repo",
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
                                repos, cfg, root_path, out))
            return false;
    }

    return true;
}

bool
resolve_install_closure(char *const *requested, size_t requested_count,
                        struct db_handle *db, const struct repo_list *repos,
                        const struct tulpar_config *cfg, const char *root_path,
                        struct pkg_set *out)
{
    for (size_t i = 0; i < requested_count; i++)
    {
        const char *arg = requested[i];
        struct package *pkg = NULL;

        if (ends_with(arg, ".apg") && file_exists(arg))
        {
            ui_debugf("%s is a local archive, parsing directly", arg);
            pkg = parse_package(arg, root_path);
            if (!pkg)
            {
                ui_errorf("failed to read package archive %s", arg);
                return false;
            }
        }
        else
        {
            ui_debugf("%s is not a local archive, resolving by name", arg);
            pkg = resolve_fetch_by_name(arg, VER_OP_ANY, NULL, repos, cfg,
                                        root_path);
            if (!pkg)
            {
                ui_errorf("package %s not found in any configured repo", arg);
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
                                    repos, cfg, root_path, out))
                return false;
        }
    }

    return true;
}
