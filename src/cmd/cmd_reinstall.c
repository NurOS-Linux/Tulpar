// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <apg/db.h>
#include <apg/package.h>
#include <apg/transaction.h>

#include "cmd_reinstall.h"
#include "cmd_common.h"
#include "resolve.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../repo/repo.h"
#include "../i18n.h"

#define USAGE                                                                  \
    "tulpar reinstall [--dest <path>] [-y] [--require-signature] "             \
    "[--dry-run] <package|file.apg>..."

static bool
ends_with(const char *s, const char *suffix)
{
    size_t slen = strlen(s);
    size_t suflen = strlen(suffix);
    return slen >= suflen && strcmp(s + slen - suflen, suffix) == 0;
}

static struct package *
load_replacement(struct db_handle *db, const char *arg,
                 const struct repo_list *repos, const struct tulpar_config *cfg)
{
    if (ends_with(arg, ".apg") && access(arg, F_OK) == 0)
    {
        struct package *pkg = cmd_parse_package(arg);
        if (!pkg)
        {
            ui_errorf(_("failed to read package %s: %s"), arg,
                      cmd_libapg_reason());
            return NULL;
        }
        struct package *installed = db_get(db, pkg->meta->name);
        if (!installed)
        {
            ui_errorf(_("%s is not installed"), pkg->meta->name);
            package_free(pkg);
            return NULL;
        }
        bool same = strcmp(installed->meta->version, pkg->meta->version) == 0;
        package_free(installed);
        if (!same)
        {
            ui_errorf(_("%s in %s is not the installed version"),
                      pkg->meta->name, arg);
            ui_hint(_("use 'tulpar install' to change the version"));
            package_free(pkg);
            return NULL;
        }
        return pkg;
    }

    struct package *installed = db_get(db, arg);
    if (!installed)
    {
        ui_errorf(_("%s is not installed"), arg);
        return NULL;
    }

    bool failed = false;
    struct package *pkg = resolve_fetch_named(
        arg, VER_OP_EQ, installed->meta->version, repos, cfg, &failed);
    if (!pkg && !failed)
        ui_errorf(_("%s %s not found in any configured repo"), arg,
                  installed->meta->version);
    package_free(installed);
    return pkg;
}

int
cmd_reinstall_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    bool assume_yes = false;
    bool require_sig = false;
    bool dry_run = false;
    const char **targets = calloc((size_t)argc + 1, sizeof(*targets));
    size_t target_count = 0;
    bool end_of_options = false;

    if (!targets)
        return 1;

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
            free(targets);
            return 0;
        }
        else if (!end_of_options &&
                 arg_take_value(argc, argv, &i, "dest", 'd', &value))
            dest_arg = value;
        else if (!end_of_options && arg_is(argv[i], "yes", 'y'))
            assume_yes = true;
        else if (!end_of_options && arg_is(argv[i], "require-signature", '\0'))
            require_sig = true;
        else if (!end_of_options && arg_is(argv[i], "dry-run", 'n'))
            dry_run = true;
        else if (end_of_options || argv[i][0] != '-')
            targets[target_count++] = argv[i];
        else
        {
            cmd_unknown_option(argv[i], USAGE);
            free(targets);
            return 1;
        }
    }

    if (target_count == 0)
    {
        ui_error(_("a package name or file is required"));
        cmd_print_usage(USAGE);
        free(targets);
        return 1;
    }

    struct dest_ctx dest = {0};
    if (!dest_ctx_resolve(dest_arg, cfg->db_dir, &dest))
    {
        ui_error(_("failed to resolve destination root"));
        free(targets);
        return 1;
    }

    if (!require_privilege(&dest))
    {
        dest_ctx_clear(&dest);
        free(targets);
        return 1;
    }

    struct db_handle *db = cmd_open_db(&dest, true);
    if (!db)
    {
        dest_ctx_clear(&dest);
        free(targets);
        return 1;
    }

    struct repo_list *repos = repo_list_load();
    struct pkg_set set = {0};
    struct timespec started;
    clock_gettime(CLOCK_MONOTONIC, &started);
    bool ok = true;

    for (size_t i = 0; ok && i < target_count; i++)
    {
        struct package *pkg = load_replacement(db, targets[i], repos, cfg);
        if (!pkg || pkg_set_contains(&set, pkg->meta->name))
        {
            ok = pkg != NULL;
            package_free(pkg);
            continue;
        }
        if (!pkg_set_add(&set, pkg))
        {
            package_free(pkg);
            ok = false;
        }
    }

    struct apg_trans *trans = ok ? trans_new(db) : NULL;
    if (ok && !trans)
    {
        ui_error(_("failed to allocate transaction"));
        ok = false;
    }

    if (ok)
    {
        ui_status(_("Resolved"),
                  ngettext("%zu package in %.2fs", "%zu packages in %.2fs",
                           set.count),
                  set.count, cmd_seconds_since(&started));
        cmd_warn_unsigned(set.items, set.count,
                          require_sig || cfg->require_signature);

        for (size_t i = 0; i < set.count; i++)
            trans_add_upgrade(trans, set.items[i]);

        ok = cmd_run_transaction(trans, db, &dest, cfg, assume_yes, require_sig,
                                 false, false, dry_run);
    }

    if (trans)
        trans_free(trans);
    pkg_set_free(&set);
    repo_list_free(repos);
    db_close(db);
    dest_ctx_clear(&dest);
    free(targets);
    return ok ? 0 : 1;
}
