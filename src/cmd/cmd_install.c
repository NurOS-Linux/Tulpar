// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apg/transaction.h>
#include <apg/util.h>

#include "cmd_install.h"
#include "cmd_common.h"
#include "resolve.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../repo/repo.h"
#include "../util/paths.h"
#include "../i18n.h"

#define USAGE                                                                  \
    "tulpar install [--dest <path>] [-y] [--require-signature] "               \
    "[--sign <sig-path>] [--provider <name>=<package>]... [--nodeps] "         \
    "[-n|--dry-run] <package|file.apg|url|git-url>..."

static void
mark_installed_by_hand(struct db_handle *db, const struct str_list *names)
{
    for (int i = 0; i < names->count; i++)
    {
        struct package *pkg = db_get(db, names->items[i]);
        bool already = pkg && pkg->installed_by_hand;
        package_free(pkg);
        if (!pkg || already)
            continue;

        if (db_set_installed_by_hand(db, names->items[i], true))
            ui_status(_("Marked"), _("%s as installed explicitly"),
                      names->items[i]);
        else
            ui_warnf(_("cannot mark %s as installed explicitly: %s"),
                     names->items[i], cmd_libapg_reason());
    }
}

int
cmd_install_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    const char *sign_path = NULL;
    bool assume_yes = false;
    bool require_sig = false;
    bool nodeps = false;
    bool dry_run = false;
    char *positional[256];
    int positional_count = 0;
    char provider_name_buf[64][256];
    char provider_pkg_buf[64][256];
    struct provider_pref provider_prefs[64];
    size_t provider_count = 0;
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
        else if (!end_of_options && arg_is(argv[i], "require-signature", '\0'))
            require_sig = true;
        else if (!end_of_options && arg_is(argv[i], "nodeps", '\0'))
            nodeps = true;
        else if (!end_of_options && arg_is(argv[i], "dry-run", 'n'))
            dry_run = true;
        else if (!end_of_options &&
                 arg_take_value(argc, argv, &i, "sign", '\0', &value))
            sign_path = value;
        else if (!end_of_options &&
                 arg_take_value(argc, argv, &i, "provider", '\0', &value))
        {
            if (provider_count < 64)
            {
                const char *eq = strchr(value, '=');
                if (!eq || eq == value || *(eq + 1) == '\0')
                {
                    ui_errorf(_("--provider requires name=package (got %s)"),
                              value);
                    return 1;
                }
                size_t nlen = (size_t)(eq - value);
                if (nlen >= sizeof(provider_name_buf[0]))
                    nlen = sizeof(provider_name_buf[0]) - 1;
                memcpy(provider_name_buf[provider_count], value, nlen);
                provider_name_buf[provider_count][nlen] = '\0';
                snprintf(provider_pkg_buf[provider_count],
                         sizeof(provider_pkg_buf[0]), "%s", eq + 1);

                provider_prefs[provider_count].name =
                    provider_name_buf[provider_count];
                provider_prefs[provider_count].pkg_name =
                    provider_pkg_buf[provider_count];
                provider_count++;
            }
        }
        else if (end_of_options || argv[i][0] != '-')
        {
            if (positional_count < 256)
                positional[positional_count++] = argv[i];
            else
            {
                ui_errorf(_("unexpected argument: %s"), argv[i]);
                cmd_print_usage(USAGE);
                return 1;
            }
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
        ui_error(_("install requires at least one package name or .apg file"));
        cmd_print_usage(USAGE);
        return 1;
    }

    if (sign_path && positional_count != 1)
    {
        ui_error(
            _("--sign requires exactly one local .apg file, URL, or git-url "
              "argument"));
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

    if (!dest_ctx_prepare_tree(&dest))
    {
        ui_error(_("failed to prepare destination database directory"));
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

    if (nodeps)
        ui_debug("--nodeps: installing exactly the requested package(s), "
                 "skipping dependency resolution");

    struct str_list requested_installed = {0};
    struct timespec resolve_started;
    clock_gettime(CLOCK_MONOTONIC, &resolve_started);
    if (!resolve_install_closure(positional, (size_t)positional_count, db,
                                 repos, cfg, dest.root, provider_prefs,
                                 (size_t)provider_count, assume_yes, nodeps,
                                 &set, &requested_installed))
    {
        str_list_free(&requested_installed);
        pkg_set_free(&set);
        repo_list_free(repos);
        db_close(db);
        dest_ctx_clear(&dest);
        return 1;
    }

    ui_debugf("resolved closure of %zu package(s) to install", set.count);
    if (set.count > 0)
        ui_status(_("Resolved"),
                  ngettext("%zu package in %.2fs", "%zu packages in %.2fs",
                           set.count),
                  set.count, cmd_seconds_since(&resolve_started));

    if (set.count == 0)
    {
        if (!dry_run)
            mark_installed_by_hand(db, &requested_installed);
        str_list_free(&requested_installed);
        pkg_set_free(&set);
        repo_list_free(repos);
        db_close(db);
        dest_ctx_clear(&dest);
        return 0;
    }

    if (sign_path && set.count > 0)
    {
        char sig_dest[4096];
        snprintf(sig_dest, sizeof(sig_dest), "%s.sig", set.items[0]->pkg_path);
        if (!copy_file(sign_path, sig_dest))
        {
            ui_errorf(_("failed to place signature from %s at %s"), sign_path,
                      sig_dest);
            str_list_free(&requested_installed);
            pkg_set_free(&set);
            repo_list_free(repos);
            db_close(db);
            dest_ctx_clear(&dest);
            return 1;
        }
        ui_debugf("placed signature %s at %s", sign_path, sig_dest);
    }

    cmd_warn_unsigned(set.items, set.count,
                      require_sig || cfg->require_signature);

    struct apg_trans *trans = trans_new(db);
    if (!trans)
    {
        ui_error(_("failed to allocate transaction"));
        str_list_free(&requested_installed);
        pkg_set_free(&set);
        repo_list_free(repos);
        db_close(db);
        dest_ctx_clear(&dest);
        return 1;
    }

    for (size_t i = 0; i < set.count; i++)
    {
        struct package *installed = db_get(db, set.items[i]->meta->name);
        if (installed)
            trans_add_upgrade(trans, set.items[i]);
        else if (i < set.explicit_count)
            trans_add_install(trans, set.items[i]);
        else
            trans_add_candidate(trans, set.items[i]);
        package_free(installed);
    }

    bool ok = cmd_run_transaction(trans, db, &dest, cfg, assume_yes,
                                  require_sig || cfg->require_signature, nodeps,
                                  false, dry_run);

    trans_free(trans);
    if (ok && !dry_run)
        mark_installed_by_hand(db, &requested_installed);
    str_list_free(&requested_installed);
    pkg_set_free(&set);
    repo_list_free(repos);
    db_close(db);
    dest_ctx_clear(&dest);

    return ok ? 0 : 1;
}
