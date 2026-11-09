// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <apg/audit.h>

#include "cmd_history.h"
#include "cmd_common.h"
#include "json_out.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../util/paths.h"
#include "../util/since_spec.h"
#include "../i18n.h"

#define USAGE                                                                  \
    "tulpar history [--dest <path>] [--json] [--package <name>] "              \
    "[--since <spec>]"

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

int
cmd_history_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    bool json_output = false;
    const char *package_filter = NULL;
    time_t since = 0;
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
        else if (!end_of_options &&
                 arg_take_value(argc, argv, &i, "package", 'p', &value))
            package_filter = value;
        else if (!end_of_options &&
                 arg_take_value(argc, argv, &i, "since", '\0', &value))
        {
            if (!since_spec_parse(value, time(NULL), &since))
            {
                ui_errorf(_("invalid --since value: %s"), value);
                ui_hint(_("use 30m, 12h, 7d, 2w, YYYY-MM-DD or @<unix time>"));
                return 1;
            }
        }
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
    struct journal_entry **all = audit_read_all(db, &count);
    struct journal_entry **entries =
        calloc((size_t)count + 1, sizeof(*entries));
    int all_count = count;
    count = 0;
    for (int i = 0; entries && i < all_count; i++)
    {
        const char *name = journal_entry_pkg_name(all[i]);
        if (package_filter && (!name || strcmp(name, package_filter) != 0))
            continue;
        if (journal_entry_timestamp(all[i]) < since)
            continue;
        entries[count++] = all[i];
    }

    if (json_output)
    {
        json_out_emit(json_out_history(entries, (size_t)count));
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

    free(entries);
    journal_free_all(all, all_count);
    db_close(db);
    dest_ctx_clear(&dest);

    return 0;
}
