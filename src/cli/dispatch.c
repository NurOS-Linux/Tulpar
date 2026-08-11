// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <string.h>

#include "dispatch.h"

#include "../cmd/cmd_download.h"
#include "../cmd/cmd_graph.h"
#include "../cmd/cmd_history.h"
#include "../cmd/cmd_hold.h"
#include "../cmd/cmd_info.h"
#include "../cmd/cmd_install.h"
#include "../cmd/cmd_key.h"
#include "../cmd/cmd_list.h"
#include "../cmd/cmd_orphans.h"
#include "../cmd/cmd_remove.h"
#include "../cmd/cmd_repo.h"
#include "../cmd/cmd_search.h"
#include "../cmd/cmd_upgrade.h"
#include "../cmd/cmd_verify.h"

const struct command g_commands[] = {
    {"install", 'i', "install one or more packages", cmd_install_run},
    {"remove", 'r', "remove an installed package", cmd_remove_run},
    {"uninstall", 'r', "remove an installed package", cmd_remove_run},
    {"upgrade", 'u', "upgrade one or all installed packages", cmd_upgrade_run},
    {"search", 's', "search local and remote package indexes", cmd_search_run},
    {"list", 'l', "list installed packages", cmd_list_run},
    {"info", 'n', "show full metadata for a package", cmd_info_run},
    {"show", 'n', "show full metadata for a package", cmd_info_run},
    {"verify", 'v', "verify installed package files", cmd_verify_run},
    {"orphans", 'o', "list and remove orphaned packages", cmd_orphans_run},
    {"autoremove", 'o', "list and remove orphaned packages", cmd_orphans_run},
    {"hold", 'g', "block a package from upgrade/removal", cmd_hold_run},
    {"unhold", 'x', "release a held package", cmd_unhold_run},
    {"history", 'a', "show past install/remove operations", cmd_history_run},
    {"audit", 'a', "show past install/remove operations", cmd_history_run},
    {"log", 'a', "show past install/remove operations", cmd_history_run},
    {"repo", 'e', "manage configured repositories", cmd_repo_run},
    {"download", 'w', "fetch a package without installing it",
     cmd_download_run},
    {"key", 'k', "manage the trusted signing keyring", cmd_key_run},
    {"graph", 'p', "export the installed package dependency graph as DOT",
     cmd_graph_run},
};

const size_t g_command_count = sizeof(g_commands) / sizeof(g_commands[0]);

const struct command *
command_lookup(const char *token)
{
    if (!token || token[0] == '\0')
        return NULL;

    bool single_char = token[1] == '\0';

    for (size_t i = 0; i < g_command_count; i++)
    {
        if (strcmp(token, g_commands[i].name) == 0)
            return &g_commands[i];
        if (single_char && token[0] == g_commands[i].alias)
            return &g_commands[i];
    }

    return NULL;
}
