// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <string.h>

#include "cmd_verify.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../util/paths.h"
#include "../i18n.h"

#define USAGE "tulpar verify [--dest <path>]"

int
cmd_verify_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    bool end_of_options = false;

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
            return 0;
        }
        else if (!end_of_options &&
                 arg_take_value(argc, argv, &i, "dest", 'd', &value))
            dest_arg = value;
        else if (!end_of_options && argv[i][0] == '-')
        {
            ui_errorf(_("unknown option: %s"), argv[i]);
            cmd_print_usage(USAGE);
            return 1;
        }
        else
        {
            ui_errorf(_("unexpected argument: %s"), argv[i]);
            cmd_print_usage(USAGE);
            return 1;
        }
    }

    struct dest_ctx dest = {0};
    dest_ctx_resolve(dest_arg, cfg->db_dir, &dest);

    struct db_handle *db = db_open_readonly(dest.db_path);
    if (!db)
    {
        ui_error(_("no package database found"));
        dest_ctx_clear(&dest);
        return 1;
    }

    int count = 0;
    struct db_verify_issue *issues = db_verify(db, dest.root, &count);

    if (count == 0)
    {
        ui_success(_("all installed packages verified successfully"));
    }
    else
    {
        for (int i = 0; i < count; i++)
        {
            const struct db_verify_issue *issue =
                db_verify_issue_at(issues, count, i);
            int missing_count = db_verify_issue_missing_count(issue);

            ui_errorf(_("%s: %d missing file(s)"),
                      db_verify_issue_pkg_name(issue), missing_count);
            for (int j = 0; j < missing_count; j++)
                printf("    %s\n", db_verify_issue_missing_file_at(issue, j));
        }
    }

    db_verify_free(issues, count);
    db_close(db);
    dest_ctx_clear(&dest);

    return count == 0 ? 0 : 1;
}
