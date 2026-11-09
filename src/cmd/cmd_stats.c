// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apg/db.h>
#include <apg/package.h>

#include "cmd_stats.h"
#include "cmd_common.h"
#include "json_out.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../util/paths.h"
#include "../i18n.h"

#define USAGE "tulpar stats [--dest <path>] [--json]"

int
cmd_stats_run(int argc, char **argv, struct tulpar_config *cfg)
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

    if (npos > 0)
    {
        ui_errorf(_("unexpected argument: %s"), pos[0]);
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
    struct db_stats raw = {0};
    int pkg_count = 0;
    struct package **pkgs = db_list(db, &pkg_count);
    int orphan_count = 0;
    char **orphans = db_get_orphans_transitive(db, &orphan_count);

    if (!db_stats(db, &raw))
        rc = 1;

    struct stats_summary summary = {
        .packages = raw.package_count,
        .files = raw.file_count,
        .orphans = orphan_count,
    };
    for (int i = 0; pkgs && i < pkg_count; i++)
    {
        if (pkgs[i]->installed_by_hand)
            summary.explicit_count++;
        else
            summary.dependencies++;
        if (pkgs[i]->held)
            summary.held++;
    }

    if (rc != 0)
        ui_error(_("failed to read database statistics"));
    else if (json_output)
        json_out_emit(json_out_stats(&summary));
    else
    {
        const char *labels[] = {_("Packages"), _("Explicit"), _("Dependencies"),
                                _("Held"),     _("Orphans"),  _("Files")};
        int values[] = {summary.packages,     summary.explicit_count,
                        summary.dependencies, summary.held,
                        summary.orphans,      summary.files};
        size_t width = 0;
        for (size_t i = 0; i < 6; i++)
            if (ui_text_width(labels[i]) > width)
                width = ui_text_width(labels[i]);
        for (size_t i = 0; i < 6; i++)
        {
            char value[32];
            snprintf(value, sizeof(value), "%d", values[i]);
            ui_print_field(labels[i], width, value);
        }
    }

    for (int i = 0; pkgs && i < pkg_count; i++)
        package_free(pkgs[i]);
    free(pkgs);
    for (int i = 0; orphans && i < orphan_count; i++)
        free(orphans[i]);
    free(orphans);

    db_close(db);
    dest_ctx_clear(&dest);
    free(pos);
    return rc;
}
