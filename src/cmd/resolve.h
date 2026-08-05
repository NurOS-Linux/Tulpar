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
};

bool resolve_arg_is_url(const char *arg);

bool pkg_set_contains(const struct pkg_set *set, const char *name);
bool pkg_set_add(struct pkg_set *set, struct package *pkg);
void pkg_set_free(struct pkg_set *set);

struct package *resolve_fetch_by_name(const char *name, ver_op_t op,
                                      const char *version,
                                      const struct repo_list *repos,
                                      const struct tulpar_config *cfg,
                                      const char *root_path);

bool resolve_install_closure(char *const *requested, size_t requested_count,
                             struct db_handle *db,
                             const struct repo_list *repos,
                             const struct tulpar_config *cfg,
                             const char *root_path, struct pkg_set *out);
