// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <string.h>

#include "dispatch.h"

#include "../i18n.h"

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
    {"install", 'i', N_("install one or more packages"), cmd_install_run},
    {"remove", 'r', N_("remove an installed package"), cmd_remove_run},
    {"uninstall", 'r', N_("remove an installed package"), cmd_remove_run},
    {"upgrade", 'u', N_("upgrade one or all installed packages"),
     cmd_upgrade_run},
    {"search", 's', N_("search local and remote package indexes"),
     cmd_search_run},
    {"list", 'l', N_("list installed packages"), cmd_list_run},
    {"info", 'n', N_("show full metadata for a package"), cmd_info_run},
    {"show", 'n', N_("show full metadata for a package"), cmd_info_run},
    {"verify", 'v', N_("verify installed package files"), cmd_verify_run},
    {"orphans", 'o', N_("list and remove orphaned packages"), cmd_orphans_run},
    {"autoremove", 'o', N_("list and remove orphaned packages"),
     cmd_orphans_run},
    {"hold", 'g', N_("block a package from upgrade/removal"), cmd_hold_run},
    {"unhold", 'x', N_("release a held package"), cmd_unhold_run},
    {"history", 'a', N_("show past install/remove operations"),
     cmd_history_run},
    {"audit", 'a', N_("show past install/remove operations"), cmd_history_run},
    {"log", 'a', N_("show past install/remove operations"), cmd_history_run},
    {"repo", 'e', N_("manage configured repositories"), cmd_repo_run},
    {"download", 'w', N_("fetch a package without installing it"),
     cmd_download_run},
    {"key", 'k', N_("manage the trusted signing keyring"), cmd_key_run},
    {"graph", 'p', N_("export the installed package dependency graph as DOT"),
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
