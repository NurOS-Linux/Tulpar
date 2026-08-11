// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>

#include <yyjson.h>

#include "cmd_list.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../util/paths.h"
#include "../i18n.h"

#define USAGE "tulpar list [--dest <path>] [--json]"

int
cmd_list_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    bool json_output = false;

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
        else if (arg_is(argv[i], "json", 'j'))
            json_output = true;
    }

    struct dest_ctx dest = {0};
    dest_ctx_resolve(dest_arg, cfg->db_dir, &dest);

    struct db_handle *db = db_open_readonly(dest.db_path);
    int count = 0;
    struct package **pkgs = db ? db_list(db, &count) : NULL;

    if (json_output)
    {
        yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
        yyjson_mut_val *arr = yyjson_mut_arr(doc);
        yyjson_mut_doc_set_root(doc, arr);

        for (int i = 0; i < count; i++)
        {
            struct package *pkg = pkgs[i];
            yyjson_mut_val *obj = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_strcpy(doc, obj, "name", pkg->meta->name);
            yyjson_mut_obj_add_strcpy(doc, obj, "version", pkg->meta->version);
            yyjson_mut_obj_add_bool(doc, obj, "held", pkg->held);
            yyjson_mut_obj_add_bool(doc, obj, "explicit",
                                    pkg->installed_by_hand);
            yyjson_mut_arr_add_val(arr, obj);
        }

        char *json = yyjson_mut_write(doc, 0, NULL);
        if (json)
        {
            printf("%s\n", json);
            free(json);
        }
        yyjson_mut_doc_free(doc);
    }
    else if (count == 0)
    {
        ui_info(_("no packages installed"));
    }
    else
    {
        for (int i = 0; i < count; i++)
        {
            struct package *pkg = pkgs[i];
            printf("%s %s%s\n", pkg->meta->name, pkg->meta->version,
                   pkg->held ? " [held]" : "");
        }
    }

    for (int i = 0; i < count; i++)
        package_free(pkgs[i]);
    free(pkgs);
    if (db)
        db_close(db);
    dest_ctx_clear(&dest);

    return 0;
}
