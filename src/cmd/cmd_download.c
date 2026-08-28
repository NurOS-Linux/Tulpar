// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd_download.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../net/api.h"
#include "../repo/repo.h"
#include "../util/paths.h"
#include "../i18n.h"

#define USAGE                                                                  \
    "tulpar download [-o <path>] [--version <v>] [--arch <a>] "                \
    "[--channel <c>] <name>"

int
cmd_download_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *output = NULL;
    const char *version = NULL;
    const char *arch = NULL;
    const char *channel = NULL;
    const char *name = NULL;
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
                 arg_take_value(argc, argv, &i, "output", 'o', &value))
            output = value;
        else if (!end_of_options &&
                 arg_take_value(argc, argv, &i, "version", '\0', &value))
            version = value;
        else if (!end_of_options &&
                 arg_take_value(argc, argv, &i, "arch", '\0', &value))
            arch = value;
        else if (!end_of_options &&
                 arg_take_value(argc, argv, &i, "channel", '\0', &value))
            channel = value;
        else if (end_of_options || argv[i][0] != '-')
        {
            if (!name)
                name = argv[i];
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

    if (!name)
    {
        ui_error(_("download requires a package name"));
        cmd_print_usage(USAGE);
        return 1;
    }

    struct repo_list *repos = repo_list_load();
    struct repo_index *idx = NULL;
    const char *found_base_url = NULL;

    if (repos)
    {
        for (int i = 0; i < repos->count; i++)
        {
            idx = api_get_package(repos->urls[i], name);
            if (idx)
            {
                found_base_url = repos->urls[i];
                break;
            }
        }
    }

    if (!idx || idx->count == 0)
    {
        ui_errorf(_("package %s not found in any configured repository"), name);
        if (idx)
            repo_index_free(idx);
        repo_list_free(repos);
        return 1;
    }

    const struct repo_package *best = NULL;
    for (size_t i = 0; i < idx->count; i++)
    {
        const struct repo_package *cand = &idx->items[i];
        if (version && strcmp(cand->version, version) != 0)
            continue;
        if (arch && strcmp(cand->architecture, arch) != 0)
            continue;
        if (channel && strcmp(cand->channel, channel) != 0)
            continue;
        best = cand;
        break;
    }

    if (!best)
    {
        ui_errorf(_("no build of %s matches the requested criteria"), name);
        repo_index_free(idx);
        repo_list_free(repos);
        return 1;
    }

    char default_out[512];
    snprintf(default_out, sizeof(default_out), "%s-%s-%s.apg", best->name,
             best->version,
             best->architecture[0] ? best->architecture : "noarch");
    const char *target = output ? output : default_out;

    const char *use_channel = best->channel[0] ? best->channel : "stable";
    const char *use_arch =
        best->architecture[0] ? best->architecture : "noarch";

    ui_infof(_("downloading %s %s to %s"), best->name, best->version, target);

    bool ok = api_download(found_base_url, use_channel, best->name,
                           best->version, use_arch, target, NULL, NULL);

    if (ok)
    {
        char sig_path[600];
        snprintf(sig_path, sizeof(sig_path), "%s.sig", target);
        api_download_sig(found_base_url, use_channel, best->name, best->version,
                         use_arch, sig_path);
        ui_successf(_("downloaded %s"), target);
    }
    else
    {
        ui_errorf(_("failed to download %s"), target);
    }

    repo_index_free(idx);
    repo_list_free(repos);
    (void)cfg;
    return ok ? 0 : 1;
}
