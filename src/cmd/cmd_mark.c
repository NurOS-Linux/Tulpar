// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd_mark.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../util/paths.h"
#include "../i18n.h"

#define USAGE "tulpar mark [--dest <path>] (--auto | --explicit) <name>..."

int
cmd_mark_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    bool mark_auto = false;
    bool mark_explicit = false;
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
        else if (!end_of_options && arg_is(argv[i], "auto", 'a'))
            mark_auto = true;
        else if (!end_of_options && arg_is(argv[i], "explicit", 'e'))
            mark_explicit = true;
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

    if (mark_auto == mark_explicit)
    {
        ui_error(_("exactly one of --auto or --explicit is required"));
        cmd_print_usage(USAGE);
        free(pos);
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
        free(pos);
        return 1;
    }

    int rc = 0;
    for (size_t i = 0; i < npos; i++)
    {
        if (db_set_installed_by_hand(db, pos[i], !mark_auto))
            ui_status(mark_auto ? _("Marked auto") : _("Marked explicit"), "%s",
                      pos[i]);
        else
        {
            struct package *known = db_get(db, pos[i]);
            if (!known)
                ui_errorf(_("package %s is not installed"), pos[i]);
            else
                ui_errorf(_("failed to mark %s: %s"), pos[i],
                          cmd_libapg_reason());
            package_free(known);
            rc = 1;
        }
    }

    db_close(db);
    dest_ctx_clear(&dest);
    free(pos);
    return rc;
}
