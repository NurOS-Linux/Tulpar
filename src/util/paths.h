// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>

char *path_join(const char *a, const char *b);

bool mkdir_p(const char *path);

bool copy_file(const char *src, const char *dst);

bool remove_dir_recursive(const char *path);

struct dest_ctx
{
    char *root;
    char *db_path;
    bool is_host_root;
};

bool dest_ctx_resolve(const char *dest_arg, const char *host_db_path,
                      struct dest_ctx *out);

void dest_ctx_clear(struct dest_ctx *ctx);

bool dest_ctx_prepare_tree(const struct dest_ctx *ctx);

bool require_privilege(const struct dest_ctx *ctx);

char *home_config_path(const char *rel);
