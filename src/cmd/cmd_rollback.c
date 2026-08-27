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
        else if (arg_is(argv[i], "yes", 'y'))
            assume_yes = true;
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

    struct apg_trans *trans = trans_new(db);
    if (!trans)
    {
        ui_error(_("failed to allocate transaction"));
        free(batch);
        journal_free_all(entries, total_entries);
        db_close(db);
        dest_ctx_clear(&dest);
        return 1;
    }

    struct repo_list *repos = repo_list_load();
    struct pkg_set collected = {0};
    bool has_actions = false;

    for (int i = 0; i < batch_count; i++)
    {
        struct journal_entry *e = batch[i];
        journal_op_t op = journal_entry_op(e);
        const char *pkg_name = journal_entry_pkg_name(e);
        const char *pkg_version = journal_entry_pkg_version(e);

        if (!pkg_name)
            continue;

        if (op == JOURNAL_INSTALL)
        {
            trans_add_remove(trans, pkg_name);
            has_actions = true;
        }
        else if (op == JOURNAL_REMOVE)
        {
            char spec[256];
            if (pkg_version && pkg_version[0] != '\0')
                snprintf(spec, sizeof(spec), "%s=%s", pkg_name, pkg_version);
            else
                snprintf(spec, sizeof(spec), "%s", pkg_name);

            char *targets[] = {spec};
            struct pkg_set closure = {0};
            bool ok = resolve_install_closure(
                targets, 1, db, repos, cfg, dest.root, NULL, 0, true, &closure);
            if (ok)
            {
                for (size_t c = 0; c < closure.count; c++)
                {
                    trans_add_install(trans, closure.items[c]);
                    pkg_set_add(&collected, closure.items[c]);
                    has_actions = true;
                }
                free(closure.items);
            }
            else
            {
                ui_errorf(_("could not resolve package %s for rollback"), spec);
            }
        }
    }

    if (repos)
        repo_list_free(repos);
    free(batch);
    journal_free_all(entries, total_entries);

    if (!has_actions)
    {
        ui_info(_("nothing to rollback"));
        pkg_set_free(&collected);
        trans_free(trans);
        db_close(db);
        dest_ctx_clear(&dest);
        return 0;
    }

    bool ok = cmd_run_transaction(trans, &dest, cfg, assume_yes,
                                  cfg->require_signature);

    pkg_set_free(&collected);
    trans_free(trans);
    db_close(db);
    dest_ctx_clear(&dest);

    return ok ? 0 : 1;
}
