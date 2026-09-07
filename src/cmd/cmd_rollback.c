// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apg/audit.h>
#include <apg/db.h>
#include <apg/transaction.h>

#include "cmd_rollback.h"
#include "cmd_common.h"
#include "resolve.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../i18n.h"
#include "../repo/repo.h"
#include "../util/paths.h"

#define USAGE "tulpar rollback [--dest <path>] [-y]"

int
cmd_rollback_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    bool assume_yes = false;
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
        else if (!end_of_options && arg_is(argv[i], "yes", 'y'))
            assume_yes = true;
        else if (!end_of_options && argv[i][0] == '-')
        {
            ui_errorf(_("unknown option: %s"), argv[i]);
            cmd_print_usage(USAGE);
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
    if (!dest_ctx_resolve(dest_arg, cfg->db_dir, &dest))
    {
        ui_error(_("failed to resolve destination root"));
        return 1;
    }

    if (!require_privilege(&dest))
    {
        dest_ctx_clear(&dest);
        return 1;
    }

    struct db_handle *db = db_open_readonly(dest.db_path);
    if (!db)
    {
        ui_error(_("no package database found"));
        dest_ctx_clear(&dest);
        return 1;
    }

    int total_entries = 0;
    struct journal_entry **entries = audit_read_all(db, &total_entries);

    int last_idx = -1;
    for (int i = total_entries - 1; i >= 0; i--)
    {
        if (journal_entry_status(entries[i]) == JOURNAL_STATUS_OK)
        {
            last_idx = i;
            break;
        }
    }

    if (last_idx < 0)
    {
        ui_info(_("no past successful operations to undo"));
        if (entries)
            journal_free_all(entries, total_entries);
        db_close(db);
        dest_ctx_clear(&dest);
        return 0;
    }

    time_t last_time = journal_entry_timestamp(entries[last_idx]);
    int batch_start = last_idx;
    while (batch_start > 0 &&
           journal_entry_timestamp(entries[batch_start - 1]) == last_time &&
           journal_entry_status(entries[batch_start - 1]) == JOURNAL_STATUS_OK)
    {
        batch_start--;
    }

    int batch_count = last_idx - batch_start + 1;
    struct journal_entry **batch = malloc(sizeof(*batch) * batch_count);
    if (!batch)
    {
        journal_free_all(entries, total_entries);
        db_close(db);
        dest_ctx_clear(&dest);
        return 1;
    }

    for (int i = 0; i < batch_count; i++)
        batch[i] = entries[batch_start + i];

    db_close(db);

    struct db_handle *wdb = cmd_open_db(&dest, true);
    if (!wdb)
    {
        free(batch);
        journal_free_all(entries, total_entries);
        dest_ctx_clear(&dest);
        return 1;
    }

    struct apg_trans *trans = trans_new(wdb);
    if (!trans)
    {
        ui_error(_("failed to allocate transaction"));
        free(batch);
        journal_free_all(entries, total_entries);
        db_close(wdb);
        dest_ctx_clear(&dest);
        return 1;
    }

    struct repo_list *repos = repo_list_load();
    struct pkg_set closure = {0};
    bool ok_setup = true;

    for (int i = batch_count - 1; i >= 0 && ok_setup; i--)
    {
        struct journal_entry *e = batch[i];
        journal_op_t op = journal_entry_op(e);
        const char *name = journal_entry_pkg_name(e);
        const char *version = journal_entry_pkg_version(e);

        if (op == JOURNAL_INSTALL)
        {
            trans_add_remove(trans, name);
        }
        else if (op == JOURNAL_REMOVE)
        {
            ver_op_t vop =
                version && version[0] != '\0' ? VER_OP_EQ : VER_OP_ANY;
            struct package *pkg = resolve_fetch_by_name(
                name, vop, version, repos, cfg, dest.root, NULL, 0, assume_yes);
            if (pkg)
            {
                if (!pkg_set_add(&closure, pkg))
                {
                    package_free(pkg);
                    ok_setup = false;
                }
                else
                {
                    trans_add_install(trans, pkg);
                }
            }
            else
            {
                ui_errorf(
                    _("failed to re-fetch removed package %s for rollback"),
                    name);
                ok_setup = false;
            }
        }
    }

    if (repos)
        repo_list_free(repos);
    free(batch);
    journal_free_all(entries, total_entries);

    if (!ok_setup)
    {
        trans_free(trans);
        pkg_set_free(&closure);
        db_close(wdb);
        dest_ctx_clear(&dest);
        return 1;
    }

    bool ok = cmd_run_transaction(trans, &dest, cfg, assume_yes, false, false);

    trans_free(trans);
    pkg_set_free(&closure);
    db_close(wdb);
    dest_ctx_clear(&dest);

    return ok ? 0 : 1;
}
