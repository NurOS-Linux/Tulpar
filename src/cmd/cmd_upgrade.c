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

#define USAGE                                                                  \
    "tulpar upgrade [--dest <path>] [-y] [--require-signature] "               \
    "[--exclude <name>]... [package[=version]]"

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
    char target_name_buf[256];
    const char *target_name = NULL;
    const char *target_version = NULL;
    char *exclude[64];
    int exclude_count = 0;

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
        else if (arg_take_value(argc, argv, &i, "exclude", '\0', &value))
        {
            if (exclude_count < 64)
                exclude[exclude_count++] = (char *)value;
        }
        else if (arg_is(argv[i], "yes", 'y'))
            assume_yes = true;
        else if (arg_is(argv[i], "require-signature", '\0'))
            require_sig = true;
        else if (!target_name)
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
    }

    if (exclude_count > 0 && target_name)
    {
        ui_error("--exclude only applies to a full upgrade (no target "
                 "package)");
        cmd_print_usage(USAGE);
        return 1;
    }

    struct dest_ctx dest = {0};
    if (!dest_ctx_resolve(dest_arg, cfg->db_dir, &dest))
    {
        ui_error("failed to resolve destination root");
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

    if (target_name)
    {
        struct package *installed = db_get(db, target_name);
        if (!installed)
        {
            ui_errorf("%s is not installed", target_name);
            repo_list_free(repos);
            db_close(db);
            dest_ctx_clear(&dest);
            return 1;
        }

        struct package *candidate =
            target_version
                ? resolve_fetch_by_name(target_name, VER_OP_EQ, target_version,
                                        repos, cfg, dest.root, NULL, 0,
                                        assume_yes)
                : resolve_fetch_by_name(target_name, VER_OP_GT,
                                        installed->meta->version, repos, cfg,
                                        dest.root, NULL, 0, assume_yes);
        package_free(installed);

        if (!candidate)
        {
            if (target_version)
                ui_errorf("%s %s not found in any configured repo", target_name,
                          target_version);
            else
                ui_info("already up to date");
            repo_list_free(repos);
            db_close(db);
            dest_ctx_clear(&dest);
            return target_version ? 1 : 0;
        }

        pkg_set_add(&set, candidate);
    }
    else
    {
        candidates_from = db_list(db, &candidates_count);
        for (int i = 0; i < candidates_count; i++)
        {
            if (is_excluded(candidates_from[i]->meta->name, exclude,
                            exclude_count))
                continue;

            struct package *candidate =
                resolve_fetch_by_name(candidates_from[i]->meta->name, VER_OP_GT,
                                      candidates_from[i]->meta->version, repos,
                                      cfg, dest.root, NULL, 0, assume_yes);
            if (candidate)
                pkg_set_add(&set, candidate);
        }
    }

    if (set.count == 0)
    {
        ui_info("nothing to upgrade");
        for (int i = 0; i < candidates_count; i++)
            package_free(candidates_from[i]);
        free(candidates_from);
        repo_list_free(repos);
        db_close(db);
        dest_ctx_clear(&dest);
        return 0;
    }

    for (size_t i = 0; i < set.count; i++)
        cmd_warn_if_unsigned(set.items[i]);

    struct apg_trans *trans = trans_new(db);
    if (!trans)
    {
        ui_error("failed to allocate transaction");
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

    bool ok = cmd_run_transaction(trans, &dest, cfg, assume_yes, require_sig);

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
