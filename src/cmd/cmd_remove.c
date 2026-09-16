// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <apg/db.h>
#include <apg/package.h>
#include <apg/transaction.h>

#include "cmd_remove.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../i18n.h"

#define USAGE                                                                  \
    "tulpar remove [--dest <path>] [-y] [-n|--dry-run] [--nodeps] "            \
    "<package>..."

static bool
ends_with(const char *s, const char *suffix)
{
    size_t slen = strlen(s);
    size_t suflen = strlen(suffix);
    return slen >= suflen && strcmp(s + slen - suflen, suffix) == 0;
}

static char *
resolve_remove_pkg_name(const char *arg, struct db_handle *db,
                        const char *root_path)
{
    struct package *installed = db_get(db, arg);
    if (installed)
    {
        char *name = strdup(installed->meta->name);
        package_free(installed);
        return name;
    }

    if (ends_with(arg, ".apg") && access(arg, F_OK) == 0)
    {
        struct package *pkg = parse_package(arg, root_path);
        if (pkg && pkg->meta && pkg->meta->name)
        {
            struct package *chk = db_get(db, pkg->meta->name);
            if (chk)
            {
                char *name = strdup(chk->meta->name);
                package_free(chk);
                package_free(pkg);
                return name;
            }
            package_free(pkg);
        }
        else if (pkg)
        {
            package_free(pkg);
        }
    }

    return NULL;
}

int
cmd_remove_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    bool assume_yes = false;
    bool dry_run = false;
    bool nodeps = false;
    char *positional[256];
    int positional_count = 0;
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
        else if (!end_of_options && arg_is(argv[i], "dry-run", 'n'))
            dry_run = true;
        else if (!end_of_options && arg_is(argv[i], "nodeps", '\0'))
            nodeps = true;
        else if (end_of_options || argv[i][0] != '-')
        {
            if (positional_count < 256)
                positional[positional_count++] = argv[i];
        }
        else
        {
            ui_errorf(_("unknown option: %s"), argv[i]);
            cmd_print_usage(USAGE);
            return 1;
        }
    }

    if (positional_count == 0)
    {
        ui_error(_("remove requires at least one package name"));
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

    char *resolved_names[256];
    for (int i = 0; i < positional_count; i++)
    {
        resolved_names[i] =
            resolve_remove_pkg_name(positional[i], db, dest.root);
        if (!resolved_names[i])
        {
            ui_errorf(_("package %s is not installed"), positional[i]);
            for (int j = 0; j < i; j++)
                free(resolved_names[j]);
            db_close(db);
            dest_ctx_clear(&dest);
            return 1;
        }
    }

    struct apg_trans *trans = trans_new(db);
    if (!trans)
    {
        ui_error(_("failed to allocate transaction"));
        for (int i = 0; i < positional_count; i++)
            free(resolved_names[i]);
        db_close(db);
        dest_ctx_clear(&dest);
        return 1;
    }

    for (int i = 0; i < positional_count; i++)
    {
        trans_add_remove(trans, resolved_names[i]);
        free(resolved_names[i]);
    }

    bool ok = cmd_run_transaction(trans, &dest, cfg, assume_yes, false, false,
                                  nodeps, dry_run);

    trans_free(trans);
    db_close(db);
    dest_ctx_clear(&dest);

    return ok ? 0 : 1;
}
