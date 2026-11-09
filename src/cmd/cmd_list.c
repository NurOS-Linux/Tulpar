// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cmd_list.h"
#include "cmd_common.h"
#include "json_out.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../util/paths.h"
#include "../i18n.h"

#define USAGE "tulpar list [--dest <path>] [--json] [pattern]"

int
cmd_list_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    const char *pattern = NULL;
    bool json_output = false;
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
        else if (!end_of_options && arg_is(argv[i], "json", 'j'))
            json_output = true;
        else if (end_of_options || argv[i][0] != '-')
        {
            if (!pattern)
                pattern = argv[i];
            else
            {
                ui_errorf(_("unexpected argument: %s"), argv[i]);
                cmd_print_usage(USAGE);
                return 1;
            }
        }
        else
        {
            cmd_unknown_option(argv[i], USAGE);
            return 1;
        }
    }

    struct dest_ctx dest = {0};
    dest_ctx_resolve(dest_arg, cfg->db_dir, &dest);

    struct db_handle *db = cmd_open_db(&dest, false);
    if (!db)
    {
        dest_ctx_clear(&dest);
        return 1;
    }

    int count = 0;
    struct package **pkgs = db_list(db, &count);

    size_t matched_count = 0;
    for (int i = 0; i < count; i++)
    {
        if (!pattern || strstr(pkgs[i]->meta->name, pattern) != NULL)
            matched_count++;
    }

    if (json_output)
    {
        struct package **shown = calloc((size_t)count + 1, sizeof(*shown));
        size_t shown_count = 0;
        for (int i = 0; shown && i < count; i++)
        {
            if (pattern && strstr(pkgs[i]->meta->name, pattern) == NULL)
                continue;
            shown[shown_count++] = pkgs[i];
        }
        json_out_emit(json_out_package_list(shown, shown_count));
        free(shown);
    }
    else
    {
        if (matched_count == 0)
        {
            if (pattern)
                ui_info(_("no installed packages matched the pattern"));
            else
                ui_info(_("no packages installed"));
        }
        else
        {
            size_t name_width = 0;
            size_t version_width = 0;
            size_t tag_width = 0;
            for (int i = 0; i < count; i++)
            {
                if (pattern && strstr(pkgs[i]->meta->name, pattern) == NULL)
                    continue;
                size_t w = ui_text_width(pkgs[i]->meta->name);
                if (w > name_width)
                    name_width = w;
                w = ui_text_width(pkgs[i]->meta->version);
                if (w > version_width)
                    version_width = w;
                if (pkgs[i]->held && ui_text_width(_("held")) > tag_width)
                    tag_width = ui_text_width(_("held"));
            }

            for (int i = 0; i < count; i++)
            {
                if (pattern && strstr(pkgs[i]->meta->name, pattern) == NULL)
                    continue;
                ui_print_package_row(pkgs[i]->meta->name, name_width,
                                     pkgs[i]->meta->version, version_width,
                                     pkgs[i]->held ? _("held") : NULL,
                                     tag_width, pkgs[i]->meta->description);
            }

            if (isatty(STDOUT_FILENO) != 0)
            {
                printf("\n");
                ui_infof(ngettext("%zu package", "%zu packages", matched_count),
                         matched_count);
            }
        }
    }

    for (int i = 0; i < count; i++)
        package_free(pkgs[i]);
    free(pkgs);

    db_close(db);
    dest_ctx_clear(&dest);
    return 0;
}
