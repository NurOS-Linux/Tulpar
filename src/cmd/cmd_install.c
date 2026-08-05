// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apg/transaction.h>

#include "cmd_install.h"
#include "cmd_common.h"
#include "resolve.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../repo/repo.h"
#include "../util/paths.h"

#define USAGE                                                                  \
    "tulpar install [--dest <path>] [-y] [--require-signature] "               \
    "[--sign <sig-path>] <package|file.apg|url|git-url>..."

static bool
ends_with_apg(const char *s)
{
    size_t len = strlen(s);
    return len >= 4 && strcmp(s + len - 4, ".apg") == 0;
}

int
cmd_install_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    const char *sign_path = NULL;
    bool assume_yes = false;
    bool require_sig = false;
    char *positional[256];
    int positional_count = 0;

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
        else if (arg_take_value(argc, argv, &i, "sign", '\0', &value))
            sign_path = value;
        else if (arg_is(argv[i], "yes", 'y'))
            assume_yes = true;
        else if (arg_is(argv[i], "require-signature", '\0'))
            require_sig = true;
        else if (positional_count < 256)
            positional[positional_count++] = argv[i];
    }

    if (positional_count == 0)
    {
        ui_error("install requires at least one package name or .apg file");
        cmd_print_usage(USAGE);
        return 1;
    }

    if (sign_path &&
        (positional_count != 1 ||
         (!ends_with_apg(positional[0]) && !resolve_arg_is_url(positional[0]) &&
          !resolve_arg_is_git_url(positional[0]))))
    {
        ui_error("--sign requires exactly one local .apg file, URL, or "
                 "git-url argument");
        cmd_print_usage(USAGE);
        return 1;
    }

    struct dest_ctx dest = {0};
    if (!dest_ctx_resolve(dest_arg, cfg->db_dir, &dest))
    {
        ui_error("failed to resolve destination root");
        return 1;
    }

    ui_debugf("target root resolved to %s", dest.root);

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
    ui_debugf("loaded %d configured repo(s)", repos->count);
    struct pkg_set set = {0};

    if (!resolve_install_closure(positional, (size_t)positional_count, db,
                                 repos, cfg, dest.root, &set))
    {
        pkg_set_free(&set);
        repo_list_free(repos);
        db_close(db);
        dest_ctx_clear(&dest);
        return 1;
    }

    ui_debugf("resolved closure of %zu package(s) to install", set.count);

    if (sign_path && set.count > 0)
    {
        char sig_dest[4096];
        snprintf(sig_dest, sizeof(sig_dest), "%s.sig", set.items[0]->pkg_path);
        if (!copy_file(sign_path, sig_dest))
        {
            ui_errorf("failed to place signature from %s at %s", sign_path,
                      sig_dest);
            pkg_set_free(&set);
            repo_list_free(repos);
            db_close(db);
            dest_ctx_clear(&dest);
            return 1;
        }
        ui_debugf("placed signature %s at %s", sign_path, sig_dest);
    }

    for (size_t i = 0; i < set.count; i++)
        cmd_warn_if_unsigned(set.items[i]);

    struct apg_trans *trans = trans_new(db);
    if (!trans)
    {
        ui_error("failed to allocate transaction");
        pkg_set_free(&set);
        repo_list_free(repos);
        db_close(db);
        dest_ctx_clear(&dest);
        return 1;
    }

    for (size_t i = 0; i < set.count; i++)
        trans_add_install(trans, set.items[i]);

    bool ok = cmd_run_transaction(trans, &dest, cfg, assume_yes, require_sig);

    trans_free(trans);
    pkg_set_free(&set);
    repo_list_free(repos);
    db_close(db);
    dest_ctx_clear(&dest);

    return ok ? 0 : 1;
}
