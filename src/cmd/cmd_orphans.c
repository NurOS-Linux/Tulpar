// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apg/transaction.h>

#include "cmd_orphans.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../i18n.h"

#define USAGE "tulpar orphans [--dest <path>] [-y] [--dry-run]"

int
cmd_orphans_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    bool assume_yes = false;
    bool dry_run = false;
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
        else if (!end_of_options && arg_is(argv[i], "yes", 'y'))
            assume_yes = true;
        else if (!end_of_options && arg_is(argv[i], "dry-run", '\0'))
            dry_run = true;
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

    if (!require_privilege(&dest))
    {
        dest_ctx_clear(&dest);
        return 1;
    }

    struct db_handle *db = cmd_open_db(&dest, true);
    if (!db)
    {
        dest_ctx_clear(&dest);
        return 1;
    }

    int count = 0;
    char **orphans = db_get_orphans(db, &count);

    if (count == 0)
    {
        ui_info(_("no orphaned packages found"));
        db_close(db);
        dest_ctx_clear(&dest);
        return 0;
    }

    ui_warnf(_("%d orphaned package(s) found"), count);

    struct apg_trans *trans = trans_new(db);
    if (!trans)
    {
        ui_error(_("failed to allocate transaction"));
        for (int i = 0; i < count; i++)
            free(orphans[i]);
        free(orphans);
        db_close(db);
        dest_ctx_clear(&dest);
        return 1;
    }

    for (int i = 0; i < count; i++)
    {
        trans_add_remove(trans, orphans[i]);
        free(orphans[i]);
    }
    free(orphans);

    bool ok =
        cmd_run_transaction(trans, &dest, cfg, assume_yes, false, false, dry_run);

    trans_free(trans);
    db_close(db);
    dest_ctx_clear(&dest);

    return ok ? 0 : 1;
}
