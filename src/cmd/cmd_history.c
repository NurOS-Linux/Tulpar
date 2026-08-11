// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <apg/audit.h>
#include <yyjson.h>

#include "cmd_history.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../util/paths.h"
#include "../i18n.h"

#define USAGE "tulpar history [--dest <path>] [--json]"

static const char *
op_name(journal_op_t op)
{
    switch (op)
    {
    case JOURNAL_INSTALL:
        return "install";
    case JOURNAL_REMOVE:
        return "remove";
    case JOURNAL_ROLLBACK:
        return "rollback";
    default:
        return "?";
    }
}

static const char *
status_name(journal_status_t status)
{
    return status == JOURNAL_STATUS_OK ? "ok" : "failed";
}

int
cmd_history_run(int argc, char **argv, struct tulpar_config *cfg)
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
    if (!db)
    {
        ui_error(_("no package database found"));
        dest_ctx_clear(&dest);
        return 1;
    }

    int count = 0;
    struct journal_entry **entries = audit_read_all(db, &count);

    if (json_output)
    {
        yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
        yyjson_mut_val *arr = yyjson_mut_arr(doc);
        yyjson_mut_doc_set_root(doc, arr);

        for (int i = 0; i < count; i++)
        {
            struct journal_entry *e = entries[i];
            const char *name = journal_entry_pkg_name(e);
            const char *version = journal_entry_pkg_version(e);

            yyjson_mut_val *obj = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_str(doc, obj, "op",
                                   op_name(journal_entry_op(e)));
            yyjson_mut_obj_add_strcpy(doc, obj, "name", name ? name : "");
            yyjson_mut_obj_add_strcpy(doc, obj, "version",
                                      version ? version : "");
            yyjson_mut_obj_add_str(doc, obj, "status",
                                   status_name(journal_entry_status(e)));
            yyjson_mut_obj_add_uint(doc, obj, "timestamp",
                                    (uint64_t)journal_entry_timestamp(e));
            yyjson_mut_obj_add_bool(doc, obj, "explicit",
                                    journal_entry_explicit(e));
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
        ui_info(_("no history recorded"));
    }
    else
    {
        for (int i = 0; i < count; i++)
        {
            struct journal_entry *e = entries[i];
            const char *name = journal_entry_pkg_name(e);
            const char *version = journal_entry_pkg_version(e);

            printf("%-8s %-8s %s %s\n", op_name(journal_entry_op(e)),
                   status_name(journal_entry_status(e)), name ? name : "",
                   version ? version : "");
        }
    }

    journal_free_all(entries, count);
    db_close(db);
    dest_ctx_clear(&dest);

    return 0;
}
