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
resolve_remove_pkg_name(const char *arg, struct db_handle *db)
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
        struct package_metadata *meta = package_read_metadata(arg);
        if (meta && meta->name)
        {
            struct package *chk = db_get(db, meta->name);
            if (chk)
            {
                char *name = strdup(chk->meta->name);
                package_free(chk);
                package_metadata_free(meta);
                return name;
            }
        }
        package_metadata_free(meta);
    }

    return NULL;
}

static bool
name_in(char *const *names, int count, const char *name)
{
    for (int i = 0; i < count; i++)
        if (strcmp(names[i], name) == 0)
            return true;
    return false;
}

static void
warn_broken_dependents(struct db_handle *db, char *const *names, int count)
{
    for (int i = 0; i < count; i++)
    {
        int dep_count = 0;
        char **dependents = db_get_dependents(db, names[i], &dep_count);
        for (int j = 0; j < dep_count; j++)
        {
            if (!name_in(names, count, dependents[j]))
            {
                ui_warnf(_("removing %s (--nodeps) although %s depends on it"),
                         names[i], dependents[j]);
                ui_note(_("%s may stop working"), dependents[j]);
            }
            free(dependents[j]);
        }
        free(dependents);
    }
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
            cmd_unknown_option(argv[i], USAGE);
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
        resolved_names[i] = resolve_remove_pkg_name(positional[i], db);
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

    if (nodeps)
        warn_broken_dependents(db, resolved_names, positional_count);

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

    bool ok = cmd_run_transaction(trans, db, &dest, cfg, assume_yes, false,
                                  false, nodeps, dry_run);

    trans_free(trans);
    db_close(db);
    dest_ctx_clear(&dest);

    return ok ? 0 : 1;
}
