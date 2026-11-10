// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#define _XOPEN_SOURCE 700

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "cmd_owns.h"
#include "cmd_common.h"
#include "json_out.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../util/paths.h"
#include "../i18n.h"

#define USAGE "tulpar owns [--dest <path>] [--json] <path>..."

static void
collapse_path(char *path)
{
    char *out = path;
    char *in = path;
    char *segments[PATH_MAX / 2];
    size_t depth = 0;

    while (*in)
    {
        while (*in == '/')
            in++;
        char *start = in;
        while (*in && *in != '/')
            in++;
        size_t len = (size_t)(in - start);
        if (len == 0 || (len == 1 && start[0] == '.'))
            continue;
        if (len == 2 && start[0] == '.' && start[1] == '.')
        {
            if (depth > 0)
                depth--;
            continue;
        }
        if (depth < PATH_MAX / 2)
            segments[depth++] = start;
        if (*in)
            *in++ = '\0';
    }

    if (depth == 0)
    {
        strcpy(path, "/");
        return;
    }
    for (size_t i = 0; i < depth; i++)
    {
        *out++ = '/';
        size_t seg_len = strlen(segments[i]);
        memmove(out, segments[i], seg_len);
        out += seg_len;
    }
    *out = '\0';
}

static char *
normalize_path(const struct dest_ctx *dest, const char *path)
{
    char cwd[PATH_MAX];
    char joined[PATH_MAX * 2];

    if (path[0] != '/')
    {
        if (!getcwd(cwd, sizeof(cwd)))
            return NULL;
        snprintf(joined, sizeof(joined), "%s/%s", cwd, path);
    }
    else
        snprintf(joined, sizeof(joined), "%s", path);
    collapse_path(joined);

    const char *abs = joined;
    if (!dest->is_host_root && dest->root)
    {
        char root[PATH_MAX * 2];
        snprintf(root, sizeof(root), "%s", dest->root);
        collapse_path(root);
        size_t root_len = strlen(root);
        if (root_len == 1)
            root_len = 0;
        if (strncmp(abs, root, root_len) == 0 &&
            (abs[root_len] == '/' || abs[root_len] == '\0'))
            abs += root_len;
    }

    return strdup(abs[0] ? abs : "/");
}

static char *
resolve_parent_symlinks(const char *path)
{
    const char *slash = strrchr(path, '/');
    if (!slash || slash == path)
        return NULL;

    char dir[PATH_MAX];
    size_t dir_len = (size_t)(slash - path);
    if (dir_len >= sizeof(dir))
        return NULL;
    memcpy(dir, path, dir_len);
    dir[dir_len] = '\0';

    char real[PATH_MAX];
    if (!realpath(dir, real) || strcmp(real, dir) == 0)
        return NULL;

    char out[PATH_MAX * 2];
    snprintf(out, sizeof(out), "%s%s%s", real,
             strcmp(real, "/") == 0 ? "" : "/", slash + 1);
    return strdup(out);
}

static char *
find_owner(struct db_handle *db, const struct dest_ctx *dest, char **shown)
{
    char *owner = db_owner(db, *shown);
    if (owner || !dest->is_host_root)
        return owner;

    char *resolved = resolve_parent_symlinks(*shown);
    if (!resolved)
        return NULL;
    owner = db_owner(db, resolved);
    if (owner)
    {
        free(*shown);
        *shown = resolved;
    }
    else
        free(resolved);
    return owner;
}

int
cmd_owns_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    bool json_output = false;
    const char **pos = calloc((size_t)argc + 1, sizeof(*pos));
    size_t npos = 0;
    bool end_of_options = false;

    if (!pos)
        return 1;

    for (int i = 0; i < argc; i++)
    {
        const char *value = NULL;
        if (!end_of_options && strcmp(argv[i], "--") == 0)
        {
            end_of_options = true;
            continue;
        }
        if (!end_of_options && arg_is_help(argv[i]))
        {
            cmd_print_usage(USAGE);
            free(pos);
            return 0;
        }
        else if (!end_of_options &&
                 arg_take_value(argc, argv, &i, "dest", 'd', &value))
            dest_arg = value;
        else if (!end_of_options && arg_is(argv[i], "json", 'j'))
            json_output = true;
        else if (end_of_options || argv[i][0] != '-')
            pos[npos++] = argv[i];
        else
        {
            cmd_unknown_option(argv[i], USAGE);
            free(pos);
            return 1;
        }
    }

    if (npos == 0)
    {
        ui_error(_("a file path is required"));
        cmd_print_usage(USAGE);
        return 1;
    }

    struct dest_ctx dest = {0};
    dest_ctx_resolve(dest_arg, cfg->db_dir, &dest);

    struct db_handle *db = cmd_open_db(&dest, false);
    if (!db)
    {
        dest_ctx_clear(&dest);
        free(pos);
        return 1;
    }

    int rc = 0;
    char **owners = calloc(npos, sizeof(*owners));
    char **shown = calloc(npos, sizeof(*shown));
    if (!owners || !shown)
        rc = 1;

    for (size_t i = 0; owners && shown && i < npos; i++)
    {
        shown[i] = normalize_path(&dest, pos[i]);
        owners[i] = shown[i] ? find_owner(db, &dest, &shown[i]) : NULL;
        if (!owners[i])
            rc = 1;
    }

    if (json_output && owners && shown)
        json_out_emit(json_out_owners((const char *const *)shown,
                                      (char *const *)owners, npos));
    else if (owners && shown)
    {
        for (size_t i = 0; i < npos; i++)
        {
            if (owners[i])
                printf("%s: %s\n", shown[i], owners[i]);
            else if (shown[i])
                ui_warnf(_("no package owns %s"), shown[i]);
        }
    }

    for (size_t i = 0; owners && i < npos; i++)
    {
        free(owners[i]);
        free(shown[i]);
    }
    free(owners);
    free(shown);

    db_close(db);
    dest_ctx_clear(&dest);
    free(pos);
    return rc;
}
