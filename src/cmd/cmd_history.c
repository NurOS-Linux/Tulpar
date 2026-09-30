// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
op_label(journal_op_t op)
{
    switch (op)
    {
    case JOURNAL_INSTALL:
        return _("install");
    case JOURNAL_REMOVE:
        return _("remove");
    case JOURNAL_ROLLBACK:
        return _("rollback");
    default:
        return "?";
    }
}

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
        else if (!end_of_options && argv[i][0] == '-')
        {
            cmd_unknown_option(argv[i], USAGE);
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

    struct db_handle *db = cmd_open_db(&dest, false);
    if (!db)
    {
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
        size_t op_width = 0;
        size_t what_width = 0;
        size_t tag_width = 0;
        char (*what)[512] = calloc((size_t)count, sizeof(*what));
        for (int i = 0; what && i < count; i++)
        {
            struct journal_entry *e = entries[i];
            const char *name = journal_entry_pkg_name(e);
            const char *version = journal_entry_pkg_version(e);
            snprintf(what[i], sizeof(what[i]), "%s%s%s", name ? name : "",
                     version && version[0] ? " " : "", version ? version : "");

            size_t w = ui_text_width(op_label(journal_entry_op(e)));
            if (w > op_width)
                op_width = w;
            w = ui_text_width(what[i]);
            if (w > what_width)
                what_width = w;
            if (journal_entry_status(e) != JOURNAL_STATUS_OK &&
                ui_text_width(_("failed")) > tag_width)
                tag_width = ui_text_width(_("failed"));
        }

        for (int i = 0; what && i < count; i++)
        {
            struct journal_entry *e = entries[i];
            char when[32];
            time_t ts = journal_entry_timestamp(e);
            struct tm tm_buf;
            strftime(when, sizeof(when), "%Y-%m-%d %H:%M",
                     localtime_r(&ts, &tm_buf));

            ui_print_package_row(
                op_label(journal_entry_op(e)), op_width, what[i], what_width,
                journal_entry_status(e) == JOURNAL_STATUS_OK ? NULL
                                                             : _("failed"),
                tag_width, when);
        }
        free(what);
    }

    journal_free_all(entries, count);
    db_close(db);
    dest_ctx_clear(&dest);

    return 0;
}
