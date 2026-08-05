// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "paths.h"
#include "../cli/ui.h"

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
    size_t alen = strlen(a);
    bool need_sep = alen > 0 && a[alen - 1] != '/';
    size_t blen = strlen(b);
    size_t off = 0;
    while (b[off] == '/')
        off++;
    blen -= off;

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
copy_file(const char *src, const char *dst)
{
    FILE *in = fopen(src, "rb");
    if (!in)
        return false;

    FILE *out = fopen(dst, "wb");
    if (!out)
    {
        fclose(in);
        return false;
    }

    char buf[65536];
    size_t n;
    bool ok = true;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
    {
        if (fwrite(buf, 1, n, out) != n)
        {
            ok = false;
            break;
        }
    }
    ok = ok && !ferror(in);

    fclose(in);
    fclose(out);
    return ok;
}

bool
remove_dir_recursive(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir)
        return errno == ENOENT;

    bool ok = true;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        char *child = path_join(path, ent->d_name);
        struct stat st;
        if (child && lstat(child, &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
                ok = remove_dir_recursive(child) && ok;
            else if (unlink(child) != 0)
                ok = false;
        }
        free(child);
    }

    closedir(dir);
    if (rmdir(path) != 0)
        ok = false;
    return ok;
}

static bool
path_is_root(const char *p)
{
    return p == NULL || strcmp(p, "/") == 0 || strcmp(p, "") == 0;
}

bool
dest_ctx_resolve(const char *dest_arg, const char *host_db_path,
                 struct dest_ctx *out)
{
    if (path_is_root(dest_arg))
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

    return out->root != NULL && out->db_path != NULL;
}

void
dest_ctx_clear(struct dest_ctx *ctx)
{
    if (!ctx)
        return;
    free(ctx->root);
    free(ctx->db_path);
    ctx->root = NULL;
    ctx->db_path = NULL;
}

bool
dest_ctx_prepare_tree(const struct dest_ctx *ctx)
{
    if (ctx->is_host_root)
        return mkdir_p(ctx->db_path);

    if (!mkdir_p(ctx->root))
        return false;
    return mkdir_p(ctx->db_path);
}

bool
require_privilege(const struct dest_ctx *ctx)
{
    if (!ctx->is_host_root)
        return true;

    if (geteuid() == 0)
        return true;

    ui_error("this operation modifies the host root and requires "
             "root privileges (use --dest to target a user-owned root)");
    return false;
}

char *
home_config_path(const char *rel)
{
    const char *home = getenv("HOME");
    if (!home || home[0] == '\0')
        return NULL;

    char *cfg = path_join(home, ".config");
    if (!cfg)
        return NULL;
    char *full = path_join(cfg, rel);
    free(cfg);
    return full;
}
