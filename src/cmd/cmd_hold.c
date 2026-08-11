// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include "cmd_hold.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../i18n.h"

static int
run_set_hold(int argc, char **argv, struct tulpar_config *cfg, bool held)
{
    const char *usage = held ? "tulpar hold [--dest <path>] <name>"
                             : "tulpar unhold [--dest <path>] <name>";
    const char *dest_arg = NULL;
    const char *name = NULL;

    for (int i = 0; i < argc; i++)
    {
        const char *value = NULL;
        if (arg_is_help(argv[i]))
        {
            cmd_print_usage(usage);
            return 0;
        }
        else if (arg_take_value(argc, argv, &i, "dest", 'd', &value))
            dest_arg = value;
        else if (!name)
            name = argv[i];
    }

    if (!name)
    {
        ui_error(_("a package name is required"));
        cmd_print_usage(usage);
        return 1;
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

    bool ok = db_set_hold(db, name, held);
    if (ok)
        ui_successf(_("%s is now %s"), name, held ? "held" : "unheld");
    else
        ui_errorf(_("failed to update hold state for %s (not installed?)"),
                  name);

    db_close(db);
    dest_ctx_clear(&dest);

    return ok ? 0 : 1;
}

int
cmd_hold_run(int argc, char **argv, struct tulpar_config *cfg)
{
    return run_set_hold(argc, argv, cfg, true);
}

int
cmd_unhold_run(int argc, char **argv, struct tulpar_config *cfg)
{
    return run_set_hold(argc, argv, cfg, false);
}
