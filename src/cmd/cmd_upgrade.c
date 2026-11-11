// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>

#include <apg/transaction.h>

#include "cmd_upgrade.h"
#include "cmd_common.h"
#include "resolve.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../repo/repo.h"
#include "../i18n.h"

#define USAGE                                                                  \
    "tulpar upgrade [--dest <path>] [-y] [--require-signature] "               \
    "[--exclude <name>]... [--dry-run] [package[=version]]"

static bool
is_excluded(const char *name, char *const *exclude, int exclude_count)
{
    for (int i = 0; i < exclude_count; i++)
        if (strcmp(name, exclude[i]) == 0)
            return true;
    return false;
}

int
cmd_upgrade_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    bool assume_yes = false;
    bool require_sig = false;
    bool dry_run = false;
    char target_name_buf[256];
    const char *target_name = NULL;
    const char *target_version = NULL;
    char *exclude[64];
    int exclude_count = 0;
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
        else if (!end_of_options &&
                 arg_take_value(argc, argv, &i, "exclude", '\0', &value))
        {
            if (exclude_count < 64)
                exclude[exclude_count++] = (char *)value;
        }
        else if (!end_of_options && arg_is(argv[i], "yes", 'y'))
            assume_yes = true;
        else if (!end_of_options && arg_is(argv[i], "require-signature", '\0'))
            require_sig = true;
        else if (!end_of_options && arg_is(argv[i], "dry-run", '\0'))
            dry_run = true;
        else if (end_of_options || argv[i][0] != '-')
        {
            if (!target_name)
            {
                const char *eq = strchr(argv[i], '=');
                if (eq)
                {
                    size_t namelen = (size_t)(eq - argv[i]);
                    if (namelen >= sizeof(target_name_buf))
                        namelen = sizeof(target_name_buf) - 1;
                    memcpy(target_name_buf, argv[i], namelen);
                    target_name_buf[namelen] = '\0';
                    target_name = target_name_buf;
                    target_version = eq + 1;
                }
                else
                    target_name = argv[i];
            }
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

    if (exclude_count > 0 && target_name)
    {
        ui_error(_("--exclude only applies to a full upgrade (no target "
                   "package)"));
        cmd_print_usage(USAGE);
        return 1;
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

    struct db_handle *db = cmd_open_db(&dest, true);
    if (!db)
    {
        dest_ctx_clear(&dest);
        return 1;
    }

    struct repo_list *repos = repo_list_load();
    struct pkg_set set = {0};

    struct package **candidates_from = NULL;
    int candidates_count = 0;
    bool fetch_failed = false;
    struct timespec resolve_started;
    clock_gettime(CLOCK_MONOTONIC, &resolve_started);

    if (target_name)
    {
        struct package *installed = db_get(db, target_name);
        if (!installed)
        {
            ui_errorf(_("package %s is not installed"), target_name);
            repo_list_free(repos);
            db_close(db);
            dest_ctx_clear(&dest);
            return 1;
        }

        bool failed = false;
        struct package *candidate =
            target_version
                ? resolve_fetch_named(target_name, VER_OP_EQ, target_version,
                                      repos, cfg, &failed)
                : resolve_fetch_named(target_name, VER_OP_GT,
                                      installed->meta->version, repos, cfg,
                                      &failed);

        if (!candidate)
        {
            if (target_version && !failed)
                ui_errorf(_("%s %s not found in any configured repo"),
                          target_name, target_version);
            else if (!failed)
                ui_infof(_("%s %s is already installed and up to date"),
                         installed->meta->name, installed->meta->version);
            package_free(installed);
            repo_list_free(repos);
            db_close(db);
            dest_ctx_clear(&dest);
            return target_version || failed ? 1 : 0;
        }

        package_free(installed);
        pkg_set_add(&set, candidate);
    }
    else
    {
        candidates_from = db_list(db, &candidates_count);
        struct remote_pick *picks =
            candidates_count > 0
                ? calloc((size_t)candidates_count, sizeof(*picks))
                : NULL;
        size_t pick_count = 0;
        for (int i = 0; picks && i < candidates_count; i++)
        {
            if (is_excluded(candidates_from[i]->meta->name, exclude,
                            exclude_count))
                continue;
            if (resolve_lookup_named(candidates_from[i]->meta->name, VER_OP_GT,
                                     candidates_from[i]->meta->version, repos,
                                     cfg, &picks[pick_count]))
                pick_count++;
        }

        struct package **fetched =
            pick_count > 0 ? calloc(pick_count, sizeof(*fetched)) : NULL;
        if (pick_count > 0 &&
            (!fetched || !resolve_download(picks, pick_count, cfg, fetched)))
            fetch_failed = true;
        for (size_t i = 0; !fetch_failed && i < pick_count; i++)
            pkg_set_add(&set, fetched[i]);
        free(fetched);
        free(picks);
    }

    if (set.count == 0)
    {
        if (!fetch_failed)
            ui_info(_("nothing to upgrade"));
        for (int i = 0; i < candidates_count; i++)
            package_free(candidates_from[i]);
        free(candidates_from);
        repo_list_free(repos);
        db_close(db);
        dest_ctx_clear(&dest);
        return fetch_failed ? 1 : 0;
    }

    ui_status(
        _("Resolved"),
        ngettext("%zu package in %.2fs", "%zu packages in %.2fs", set.count),
        set.count, cmd_seconds_since(&resolve_started));

    cmd_warn_unsigned(set.items, set.count,
                      require_sig || cfg->require_signature);

    struct apg_trans *trans = trans_new(db);
    if (!trans)
    {
        ui_error(_("failed to allocate transaction"));
        pkg_set_free(&set);
        for (int i = 0; i < candidates_count; i++)
            package_free(candidates_from[i]);
        free(candidates_from);
        repo_list_free(repos);
        db_close(db);
        dest_ctx_clear(&dest);
        return 1;
    }

    for (size_t i = 0; i < set.count; i++)
        trans_add_upgrade(trans, set.items[i]);

    bool ok = cmd_run_transaction(trans, db, &dest, cfg, assume_yes,
                                  require_sig, false, false, dry_run);

    trans_free(trans);
    pkg_set_free(&set);
    for (int i = 0; i < candidates_count; i++)
        package_free(candidates_from[i]);
    free(candidates_from);
    repo_list_free(repos);
    db_close(db);
    dest_ctx_clear(&dest);

    return ok ? 0 : 1;
}
