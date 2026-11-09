// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd_files.h"
#include "cmd_common.h"
#include "json_out.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../util/paths.h"
#include "../i18n.h"

#define USAGE "tulpar files [--dest <path>] [--json] <name>"

int
cmd_files_run(int argc, char **argv, struct tulpar_config *cfg)
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
        ui_error(_("a package name is required"));
        cmd_print_usage(USAGE);
        return 1;
    }

    if (npos > 1)
    {
        ui_errorf(_("unexpected argument: %s"), pos[1]);
        cmd_print_usage(USAGE);
        free(pos);
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
    struct package *pkg = db_get(db, pos[0]);
    if (!pkg)
    {
        ui_errorf(_("package %s is not installed"), pos[0]);
        rc = 1;
    }
    else
    {
        if (json_output)
            json_out_emit(json_out_files(pkg));
        else
            for (int i = 0; i < pkg->package_files.count; i++)
                printf("%s\n", pkg->package_files.items[i]);
        package_free(pkg);
    }

    db_close(db);
    dest_ctx_clear(&dest);
    free(pos);
    return rc;
}
