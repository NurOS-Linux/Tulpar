// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <apg/db.h>
#include <apg/package.h>
#include <apg/version.h>

#include "../config/config.h"
#include "../repo/repo.h"

struct pkg_set
{
    struct package **items;
    size_t count;
    size_t cap;
    size_t explicit_count;
};

struct provider_pref
{
    const char *name;
    const char *pkg_name;
};

bool resolve_arg_is_url(const char *arg);
bool resolve_arg_is_git_url(const char *arg);

bool pkg_set_contains(const struct pkg_set *set, const char *name);
bool pkg_set_add(struct pkg_set *set, struct package *pkg);
void pkg_set_free(struct pkg_set *set);

bool resolve_dep_installed(struct db_handle *db,
                           const struct dep_constraint *dep);

struct remote_pick
{
    char name[256];
    char version[128];
    char base_url[1024];
    char label[400];
    char url[2048];
    char sig_url[2056];
    char dest_path[4096];
    char sig_path[4104];
};

bool resolve_lookup_named(const char *name, ver_op_t op, const char *version,
                          const struct repo_list *repos,
                          const struct tulpar_config *cfg,
                          struct remote_pick *out);

bool resolve_download(const struct remote_pick *picks, size_t count,
                      const struct tulpar_config *cfg, struct package **out);

struct package *resolve_fetch_named(const char *name, ver_op_t op,
                                    const char *version,
                                    const struct repo_list *repos,
                                    const struct tulpar_config *cfg,
                                    bool *failed);

bool resolve_install_closure(
    char *const *requested, size_t requested_count, struct db_handle *db,
    const struct repo_list *repos, const struct tulpar_config *cfg,
    const struct provider_pref *prefs, size_t pref_count, bool assume_yes,
    bool nodeps, struct pkg_set *out, struct str_list *requested_installed);
