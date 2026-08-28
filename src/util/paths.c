// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "paths.h"
#include "../cli/ui.h"
#include "../i18n.h"

static bool
mkdir_or_exists(const char *path)
{
    if (mkdir(path, 0755) == 0)
        return true;
    return errno == EEXIST;
}

char *
path_join(const char *a, const char *b)
{
    if (!a && !b)
        return NULL;
    if (!a)
        return strdup(b);
    if (!b)
        return strdup(a);

    size_t alen = strlen(a);
    size_t blen = strlen(b);

    while (alen > 0 && a[alen - 1] == '/')
        alen--;

    size_t off = 0;
    while (off < blen && b[off] == '/')
        off++;
    blen -= off;

    bool need_sep = (alen > 0 && blen > 0);
    size_t total = alen + (need_sep ? 1 : 0) + blen + 1;
    char *out = malloc(total);
    if (!out)
        return NULL;

    memcpy(out, a, alen);
    size_t pos = alen;
    if (need_sep)
        out[pos++] = '/';
    memcpy(out + pos, b + off, blen);
    pos += blen;
    out[pos] = '\0';
    return out;
}

bool
mkdir_p(const char *path)
{
    size_t len = strlen(path);
    if (len == 0 || len >= 4096)
        return false;

    char buf[4096];
    memcpy(buf, path, len + 1);

    for (size_t i = 1; i < len; i++)
    {
        if (buf[i] != '/')
            continue;
        buf[i] = '\0';
        if (!mkdir_or_exists(buf))
        {
            buf[i] = '/';
            return false;
        }
        buf[i] = '/';
    }

    if (!mkdir_or_exists(buf))
        return false;

    struct stat st;
    return stat(path, &st) == 0;
}

bool
dest_ctx_resolve(const char *dest_arg, const char *host_db_path,
                 struct dest_ctx *out)
{
    if (!out)
        return false;

    memset(out, 0, sizeof(*out));

    if (!dest_arg || dest_arg[0] == '\0')
    {
        out->root = strdup("/");
        out->db_path = strdup(host_db_path);
        out->is_host_root = true;
    }
    else
    {
        out->root = strdup(dest_arg);
        out->db_path = path_join(dest_arg, "var/lib/apg/db");
        out->is_host_root = false;
    }

    return out->root && out->db_path;
}

void
dest_ctx_clear(struct dest_ctx *ctx)
{
    if (!ctx)
        return;

    free(ctx->root);
    free(ctx->db_path);
    memset(ctx, 0, sizeof(*ctx));
}

bool
dest_ctx_prepare_tree(const struct dest_ctx *ctx)
{
    if (!ctx || !ctx->db_path)
        return false;
    return mkdir_p(ctx->db_path);
}

bool
require_privilege(const struct dest_ctx *ctx)
{
    if (ctx && !ctx->is_host_root)
        return true;

    if (geteuid() != 0)
    {
        ui_error(_("operation requires root privileges (try with sudo)"));
        return false;
    }
    return true;
}

char *
home_config_path(const char *rel)
{
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0] != '\0')
        return path_join(xdg, rel);

    const char *home = getenv("HOME");
    if (!home || home[0] == '\0')
        return NULL;

    char *cfg_dir = path_join(home, ".config");
    if (!cfg_dir)
        return NULL;

    char *out = path_join(cfg_dir, rel);
    free(cfg_dir);
    return out;
}
