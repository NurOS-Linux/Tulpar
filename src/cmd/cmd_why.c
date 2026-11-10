// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd_why.h"
#include "cmd_common.h"
#include "json_out.h"
#include "why_chain.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../util/paths.h"
#include "../i18n.h"

#define USAGE "tulpar why [--dest <path>] [--json] <name>"

int
cmd_why_run(int argc, char **argv, struct tulpar_config *cfg)
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
        bool explicit_install = pkg->installed_by_hand;
        int dep_count = 0;
        char **dependents = db_get_dependents(db, pos[0], &dep_count);
        int chain_count = 0;
        char **chain =
            explicit_install ? NULL : why_find_chain(db, pos[0], &chain_count);

        bool orphan = false;
        int orphan_count = 0;
        char **orphans = explicit_install
                             ? NULL
                             : db_get_orphans_transitive(db, &orphan_count);
        for (int i = 0; orphans && i < orphan_count; i++)
            if (strcmp(orphans[i], pos[0]) == 0)
                orphan = true;
        why_free_names(orphans, orphan_count);

        if (json_output)
        {
            json_out_emit(json_out_why(pos[0], explicit_install, orphan,
                                       dependents, dep_count, chain,
                                       chain_count));
        }
        else if (explicit_install)
        {
            ui_infof(_("%s was installed explicitly"), pos[0]);
            if (dep_count > 0)
            {
                printf("%s", _("also required by:"));
                for (int i = 0; i < dep_count; i++)
                    printf(" %s", dependents[i]);
                printf("\n");
            }
        }
        else if (dep_count == 0)
        {
            ui_infof(_("%s is not required by any package"), pos[0]);
            ui_hint(_("it is an orphan and can be removed with 'tulpar "
                      "orphans'"));
        }
        else
        {
            ui_infof(_("%s was installed as a dependency"), pos[0]);
            printf("%s", _("required by:"));
            for (int i = 0; i < dep_count; i++)
                printf(" %s", dependents[i]);
            printf("\n");
            if (chain_count > 1)
            {
                printf("%s ", _("chain:"));
                for (int i = 0; i < chain_count; i++)
                    printf("%s%s", chain[i],
                           i + 1 < chain_count ? " -> " : "\n");
            }
            if (orphan)
                ui_hint(_("only unused packages require it, so it can be "
                          "removed with 'tulpar orphans'"));
        }

        why_free_names(dependents, dep_count);
        why_free_names(chain, chain_count);
        package_free(pkg);
    }

    db_close(db);
    dest_ctx_clear(&dest);
    free(pos);
    return rc;
}
