// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <apg/transaction.h>

#include "cmd_remove.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"

#define USAGE "tulpar remove [--dest <path>] [-y] <package>..."

int
cmd_remove_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    bool assume_yes = false;
    char *positional[256];
    int positional_count = 0;

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
        else if (arg_is(argv[i], "yes", 'y'))
            assume_yes = true;
        else if (positional_count < 256)
            positional[positional_count++] = argv[i];
    }

    if (positional_count == 0)
    {
        ui_error("remove requires at least one package name");
        cmd_print_usage(USAGE);
        return 1;
    }

    struct dest_ctx dest = {0};
    if (!dest_ctx_resolve(dest_arg, cfg->db_dir, &dest))
    {
        ui_error("failed to resolve destination root");
        return 1;
    }

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

    struct apg_trans *trans = trans_new(db);
    if (!trans)
    {
        ui_error("failed to allocate transaction");
        db_close(db);
        dest_ctx_clear(&dest);
        return 1;
    }

    for (int i = 0; i < positional_count; i++)
        trans_add_remove(trans, positional[i]);

    bool ok = cmd_run_transaction(trans, &dest, cfg, assume_yes, false,
                                  cfg->sign_backend);

    trans_free(trans);
    db_close(db);
    dest_ctx_clear(&dest);

    return ok ? 0 : 1;
}
