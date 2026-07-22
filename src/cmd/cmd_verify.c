// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>

#include "cmd_verify.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../util/paths.h"

#define USAGE "tulpar verify [--dest <path>]"

int
cmd_verify_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;

    for (int i = 0; i < argc; i++)
    {
        const char *value = NULL;
        if (arg_is_help(argv[i]))
        {
            cmd_print_usage(USAGE);
            return 0;
        }
        else if (arg_take_value(argc, argv, &i, "dest", 'd', &value))
            dest_arg = value;
    }

    struct dest_ctx dest = {0};
    dest_ctx_resolve(dest_arg, cfg->db_dir, &dest);

    struct db_handle *db = db_open_readonly(dest.db_path);
    if (!db)
    {
        ui_error("no package database found");
        dest_ctx_clear(&dest);
        return 1;
    }

    int count = 0;
    struct db_verify_issue *issues = db_verify(db, dest.root, &count);

    if (count == 0)
    {
        ui_success("all installed packages verified successfully");
    }
    else
    {
        for (int i = 0; i < count; i++)
        {
            ui_errorf("%s: %d missing file(s)", issues[i].pkg_name,
                      issues[i].missing_count);
            for (int j = 0; j < issues[i].missing_count; j++)
                printf("    %s\n", issues[i].missing_files[j]);
        }
    }

    db_verify_free(issues, count);
    db_close(db);
    dest_ctx_clear(&dest);

    return count == 0 ? 0 : 1;
}
