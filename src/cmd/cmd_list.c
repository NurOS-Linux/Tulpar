// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <yyjson.h>

#include "cmd_list.h"
#include "cmd_common.h"
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
            ui_errorf(_("unknown option: %s"), argv[i]);
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
    struct package **pkgs = db_list(db, &count);

    size_t matched_count = 0;
    for (int i = 0; i < count; i++)
    {
        if (!pattern || strstr(pkgs[i]->meta->name, pattern) != NULL)
            matched_count++;
    }

    if (json_output)
    {
        yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
        yyjson_mut_val *arr = yyjson_mut_arr(doc);
        yyjson_mut_doc_set_root(doc, arr);

        for (int i = 0; i < count; i++)
        {
            if (pattern && strstr(pkgs[i]->meta->name, pattern) == NULL)
                continue;
            yyjson_mut_val *obj = yyjson_mut_arr_add_obj(doc, arr);
            yyjson_mut_obj_add_strcpy(doc, obj, "name", pkgs[i]->meta->name);
            yyjson_mut_obj_add_strcpy(doc, obj, "version",
                                      pkgs[i]->meta->version);
            yyjson_mut_obj_add_strcpy(
                doc, obj, "description",
                pkgs[i]->meta->description ? pkgs[i]->meta->description : "");
            yyjson_mut_obj_add_bool(doc, obj, "installed_by_hand",
                                    pkgs[i]->installed_by_hand);
            yyjson_mut_obj_add_bool(doc, obj, "held", pkgs[i]->held);
        }

        char *json = yyjson_mut_write(doc, 0, NULL);
        if (json)
        {
            printf("%s\n", json);
            free(json);
        }
        yyjson_mut_doc_free(doc);
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
            for (int i = 0; i < count; i++)
            {
                if (pattern && strstr(pkgs[i]->meta->name, pattern) == NULL)
                    continue;
                printf("  %-16s %-12s %s\n", pkgs[i]->meta->name,
                       pkgs[i]->meta->version,
                       pkgs[i]->meta->description ? pkgs[i]->meta->description
                                                  : "");
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
