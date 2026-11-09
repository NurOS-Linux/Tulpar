// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cmd_repo.h"
#include "cmd_common.h"
#include "json_out.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../repo/repo.h"
#include "../net/http.h"
#include "../repo/repodata.h"
#include "../i18n.h"

#define USAGE                                                                  \
    "tulpar repo add <url> [--user]\n"                                         \
    "       tulpar repo remove <url> [--user]\n"                               \
    "       tulpar repo enable <url> [--user]\n"                               \
    "       tulpar repo disable <url> [--user]\n"                              \
    "       tulpar repo priority <url> <number> [--user]\n"                    \
    "       tulpar repo list [--all] [--json]\n"                               \
    "       tulpar repo update"

static char *
target_file(bool force_user)
{
    if (!force_user && geteuid() == 0)
        return repo_system_file();
    return repo_user_file();
}

static int
run_add(int argc, char **argv)
{
    bool force_user = false;
    const char *url = NULL;
    bool end_of_options = false;

    for (int i = 0; i < argc; i++)
    {
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
        else if (!end_of_options && arg_is(argv[i], "user", '\0'))
            force_user = true;
        else if (end_of_options || argv[i][0] != '-')
        {
            if (!url)
                url = argv[i];
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

    if (!url)
    {
        ui_error(_("repo add requires a URL"));
        cmd_print_usage(USAGE);
        return 1;
    }

    char *file = target_file(force_user);
    if (!file)
    {
        ui_error(_("could not determine a repo list file to write to"));
        return 1;
    }

    bool ok = repo_list_add(file, url);
    if (ok)
        ui_status(_("Added"), _("repository %s to %s"), url, file);
    else
        ui_errorf(_("failed to add repo to %s"), file);

    free(file);
    return ok ? 0 : 1;
}

static int
run_remove(int argc, char **argv)
{
    bool force_user = false;
    const char *url = NULL;
    bool end_of_options = false;

    for (int i = 0; i < argc; i++)
    {
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
        else if (!end_of_options && arg_is(argv[i], "user", '\0'))
            force_user = true;
        else if (end_of_options || argv[i][0] != '-')
        {
            if (!url)
                url = argv[i];
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

    if (!url)
    {
        ui_error(_("repo remove requires a URL"));
        cmd_print_usage(USAGE);
        return 1;
    }

    char *file = target_file(force_user);
    if (!file)
    {
        ui_error(_("could not determine a repo list file to write to"));
        return 1;
    }

    bool ok = repo_list_remove(file, url);
    if (ok)
        ui_status(_("Removed"), _("repository %s from %s"), url, file);
    else
        ui_errorf(_("repo %s was not found in %s"), url, file);

    free(file);
    return ok ? 0 : 1;
}

static int
run_change(const char *action, int argc, char **argv)
{
    bool force_user = false;
    const char *url = NULL;
    const char *number = NULL;
    bool want_number = strcmp(action, "priority") == 0;
    bool end_of_options = false;

    for (int i = 0; i < argc; i++)
    {
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
        else if (!end_of_options && arg_is(argv[i], "user", '\0'))
            force_user = true;
        else if (end_of_options || argv[i][0] != '-')
        {
            if (!url)
                url = argv[i];
            else if (want_number && !number)
                number = argv[i];
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

    if (!url || (want_number && !number))
    {
        ui_errorf(_("repo %s requires a URL%s"), action,
                  want_number ? _(" and a number") : "");
        cmd_print_usage(USAGE);
        return 1;
    }

    int priority = 0;
    if (want_number)
    {
        char *end = NULL;
        long value = strtol(number, &end, 10);
        if (end == number || *end != '\0' || value < 0 || value > 100000)
        {
            ui_errorf(_("invalid priority: %s"), number);
            ui_hint(_("use a whole number from 0 to 100000, lower is "
                      "consulted first"));
            return 1;
        }
        priority = (int)value;
    }

    char *file = NULL;
    if (!force_user)
    {
        struct repo_entries known = {0};
        repo_entries_load(&known);
        for (int i = 0; i < known.count && !file; i++)
            if (strcmp(known.items[i].url, url) == 0)
                file = known.items[i].user_file ? repo_user_file()
                                                : repo_system_file();
        repo_entries_free(&known);
    }
    if (!file)
        file = target_file(force_user);
    if (!file)
    {
        ui_error(_("could not determine a repo list file to write to"));
        return 1;
    }

    if (access(file, W_OK) != 0)
    {
        ui_errorf(_("cannot modify %s"), file);
        ui_hint(_("run as root to change the system repository list"));
        free(file);
        return 1;
    }

    bool ok;
    if (want_number)
        ok = repo_set_priority(file, url, priority);
    else
        ok = repo_set_enabled(file, url, strcmp(action, "enable") == 0);

    if (ok)
        ui_status(want_number                     ? _("Prioritised")
                  : strcmp(action, "enable") == 0 ? _("Enabled")
                                                  : _("Disabled"),
                  _("repository %s in %s"), url, file);
    else
        ui_errorf(_("repo %s was not found in %s"), url, file);

    free(file);
    return ok ? 0 : 1;
}

static int
run_list(bool show_all, bool json_output)
{
    struct repo_entries entries = {0};
    repo_entries_load(&entries);

    if (json_output)
    {
        json_out_emit(json_out_repos(entries.items, (size_t)entries.count));
        repo_entries_free(&entries);
        return 0;
    }

    int shown = 0;
    for (int i = 0; i < entries.count; i++)
    {
        const struct repo_entry *e = &entries.items[i];
        if (!e->enabled && !show_all)
            continue;
        shown++;
        if (!show_all)
            printf("%s\n", e->url);
        else
            printf("%s\t%s\t%d\n", e->enabled ? "enabled" : "disabled", e->url,
                   e->priority);
    }
    if (shown == 0)
        ui_info(_("no repositories configured"));

    repo_entries_free(&entries);
    return 0;
}

static int
run_update(struct tulpar_config *cfg)
{
    if (http_is_offline())
    {
        ui_error(_("cannot update repository indexes in offline mode"));
        return 1;
    }

    struct repo_list *list = repo_list_load();
    int failures = 0;

    for (int i = 0; i < list->count; i++)
    {
        if (repodata_refresh(list->urls[i], cfg->cache_dir))
            ui_status(_("Updated"), _("index of %s"), list->urls[i]);
        else
        {
            ui_warnf(_("failed to update %s: %s"), list->urls[i],
                     http_last_error());
            failures++;
        }
    }

    repo_list_free(list);
    return failures == 0 ? 0 : 1;
}

int
cmd_repo_run(int argc, char **argv, struct tulpar_config *cfg)
{
    if (argc == 0)
    {
        ui_error(_("repo requires a sub-action: add, remove, enable, disable, "
                   "priority, list, update"));
        cmd_print_usage(USAGE);
        return 1;
    }

    const char *action = argv[0];

    if (arg_is_help(action))
    {
        cmd_print_usage(USAGE);
        return 0;
    }

    bool is_add = strcmp(action, "add") == 0 || strcmp(action, "a") == 0;
    bool is_remove = strcmp(action, "remove") == 0 || strcmp(action, "r") == 0;
    bool is_list = strcmp(action, "list") == 0 || strcmp(action, "l") == 0;
    bool is_enable = strcmp(action, "enable") == 0;
    bool is_disable = strcmp(action, "disable") == 0;
    bool is_priority = strcmp(action, "priority") == 0;
    bool is_update = strcmp(action, "update") == 0 || strcmp(action, "u") == 0;

    if (is_add)
        return run_add(argc - 1, argv + 1);
    if (is_remove)
        return run_remove(argc - 1, argv + 1);
    if (is_enable || is_disable || is_priority)
        return run_change(action, argc - 1, argv + 1);
    if (is_list)
    {
        bool show_all = false;
        bool json_output = false;
        for (int i = 1; i < argc; i++)
        {
            if (arg_is_help(argv[i]))
            {
                cmd_print_usage(USAGE);
                return 0;
            }
            else if (arg_is(argv[i], "all", 'a'))
                show_all = true;
            else if (arg_is(argv[i], "json", 'j'))
                json_output = true;
            else
            {
                cmd_unknown_option(argv[i], USAGE);
                return 1;
            }
        }
        return run_list(show_all, json_output);
    }
    if (is_update)
    {
        for (int i = 1; i < argc; i++)
        {
            if (arg_is_help(argv[i]))
            {
                cmd_print_usage(USAGE);
                return 0;
            }
            else
            {
                cmd_unknown_option(argv[i], USAGE);
                return 1;
            }
        }
        return run_update(cfg);
    }

    ui_errorf(_("unknown repo sub-action: %s"), action);
    cmd_print_usage(USAGE);
    return 1;
}
