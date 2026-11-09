// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <string.h>

#include "cmd_verify.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../util/paths.h"
#include "../i18n.h"

#define USAGE "tulpar verify [--dest <path>] [--db]"

static void
print_database_issue(const struct db_check_issue *issue)
{
    const char *pkg = db_check_issue_pkg_name(issue);
    const char *path = db_check_issue_path(issue);
    const char *other = db_check_issue_other(issue);

    switch (db_check_issue_kind(issue))
    {
    case DB_CHECK_CORRUPT_RECORD:
        ui_errorf(_("%s: the package record cannot be read"), pkg);
        break;
    case DB_CHECK_NAME_MISMATCH:
        ui_errorf(_("%s: the record is stored under another name (%s)"), pkg,
                  other ? other : "?");
        break;
    case DB_CHECK_FILES_WITHOUT_PACKAGE:
        ui_errorf(_("%s: a file list exists but the package is not installed"),
                  pkg);
        break;
    case DB_CHECK_FILE_NOT_OWNED:
        ui_errorf(_("%s: %s is listed but has no recorded owner"), pkg, path);
        break;
    case DB_CHECK_FILE_OWNED_BY_OTHER:
        ui_errorf(_("%s: %s is listed but recorded as owned by %s"), pkg, path,
                  other ? other : "?");
        break;
    case DB_CHECK_OWNER_WITHOUT_PACKAGE:
        ui_errorf(_("%s: recorded as the owner of %s but not installed"), pkg,
                  path);
        break;
    case DB_CHECK_OWNER_NOT_LISTED:
        ui_errorf(_("%s: recorded as the owner of %s but does not list it"),
                  pkg, path);
        break;
    default:
        ui_errorf(_("%s: unknown database inconsistency"), pkg);
        break;
    }
}

static int
verify_database(struct db_handle *db)
{
    int count = 0;
    struct db_check_issue *issues = db_check(db, &count);

    if (count == 0)
    {
        ui_status(_("Verified"), _("the package database is consistent"));
        return 0;
    }

    for (int i = 0; i < count; i++)
        print_database_issue(db_check_issue_at(issues, count, i));
    ui_hint(_("reinstall the affected packages to rebuild their records"));

    db_check_free(issues, count);
    return count;
}

static int
verify_files(struct db_handle *db, const char *root)
{
    int count = 0;
    struct db_verify_issue *issues = db_verify(db, root, &count);

    if (count == 0)
    {
        ui_status(_("Verified"), _("all installed packages are intact"));
    }
    else
    {
        for (int i = 0; i < count; i++)
        {
            const struct db_verify_issue *issue =
                db_verify_issue_at(issues, count, i);
            int missing_count = db_verify_issue_missing_count(issue);

            ui_errorf(ngettext("%s: %d file is missing",
                               "%s: %d files are missing", missing_count),
                      db_verify_issue_pkg_name(issue), missing_count);
            for (int j = 0; j < missing_count; j++)
                printf("    %s\n", db_verify_issue_missing_file_at(issue, j));
        }
        ui_hint(_("reinstall the affected packages to restore their files"));
    }

    db_verify_free(issues, count);
    return count;
}

int
cmd_verify_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    bool check_database = false;
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
        else if (!end_of_options && arg_is(argv[i], "db", '\0'))
            check_database = true;
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

    int problems = verify_files(db, dest.root);
    if (check_database)
        problems += verify_database(db);

    db_close(db);
    dest_ctx_clear(&dest);

    return problems == 0 ? 0 : 1;
}
