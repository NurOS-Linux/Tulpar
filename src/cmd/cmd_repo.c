// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cmd_repo.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../repo/repo.h"
#include "../repo/repodata.h"

#define USAGE                                                                  \
    "tulpar repo add <url> [--user]\n"                                         \
    "       tulpar repo remove <url> [--user]\n"                               \
    "       tulpar repo list\n"                                                \
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

    for (int i = 0; i < argc; i++)
    {
        if (arg_is_help(argv[i]))
        {
            cmd_print_usage(USAGE);
            return 0;
        }
        else if (arg_is(argv[i], "user", '\0'))
            force_user = true;
        else if (!url)
            url = argv[i];
    }

    if (!url)
    {
        ui_error("repo add requires a URL");
        cmd_print_usage(USAGE);
        return 1;
    }

    char *file = target_file(force_user);
    if (!file)
    {
        ui_error("could not determine a repo list file to write to");
        return 1;
    }

    bool ok = repo_list_add(file, url);
    if (ok)
        ui_successf("added repo %s to %s", url, file);
    else
        ui_errorf("failed to add repo to %s", file);

    free(file);
    return ok ? 0 : 1;
}

static int
run_remove(int argc, char **argv)
{
    bool force_user = false;
    const char *url = NULL;

    for (int i = 0; i < argc; i++)
    {
        if (arg_is_help(argv[i]))
        {
            cmd_print_usage(USAGE);
            return 0;
        }
        else if (arg_is(argv[i], "user", '\0'))
            force_user = true;
        else if (!url)
            url = argv[i];
    }

    if (!url)
    {
        ui_error("repo remove requires a URL");
        cmd_print_usage(USAGE);
        return 1;
    }

    char *file = target_file(force_user);
    if (!file)
    {
        ui_error("could not determine a repo list file to write to");
        return 1;
    }

    bool ok = repo_list_remove(file, url);
    if (ok)
        ui_successf("removed repo %s from %s", url, file);
    else
        ui_errorf("repo %s was not found in %s", url, file);

    free(file);
    return ok ? 0 : 1;
}

static int
run_list(void)
{
    struct repo_list *list = repo_list_load();
    if (list->count == 0)
        ui_info("no repositories configured");
    else
        for (int i = 0; i < list->count; i++)
            printf("%s\n", list->urls[i]);

    repo_list_free(list);
    return 0;
}

static int
run_update(struct tulpar_config *cfg)
{
    struct repo_list *list = repo_list_load();
    int failures = 0;

    for (int i = 0; i < list->count; i++)
    {
        if (repodata_refresh(list->urls[i], cfg->cache_dir))
            ui_successf("updated %s", list->urls[i]);
        else
        {
            ui_warnf("failed to update %s", list->urls[i]);
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
        ui_error("repo requires a sub-action: add, remove, list, update");
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
    bool is_update = strcmp(action, "update") == 0 || strcmp(action, "u") == 0;

    if (is_add)
        return run_add(argc - 1, argv + 1);
    if (is_remove)
        return run_remove(argc - 1, argv + 1);
    if (is_list)
        return run_list();
    if (is_update)
        return run_update(cfg);

    ui_errorf("unknown repo sub-action: %s", action);
    cmd_print_usage(USAGE);
    return 1;
}
