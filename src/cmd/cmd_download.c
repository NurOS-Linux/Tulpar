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

#define USAGE                                                                  \
    "tulpar download [-o <path>] [--version <v>] [--arch <a>] "                \
    "[--channel <c>] <name>"

int
cmd_download_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *output = NULL;
    const char *version = NULL;
    const char *arch = NULL;
    const char *channel = "stable";
    const char *name = NULL;

    for (int i = 0; i < argc; i++)
    {
        const char *value = NULL;
        if (arg_is_help(argv[i]))
        {
            cmd_print_usage(USAGE);
            return 0;
        }
        else if (arg_take_value(argc, argv, &i, "output", 'o', &value))
            output = value;
        else if (arg_take_value(argc, argv, &i, "version", '\0', &value))
            version = value;
        else if (arg_take_value(argc, argv, &i, "arch", '\0', &value))
            arch = value;
        else if (arg_take_value(argc, argv, &i, "channel", '\0', &value))
            channel = value;
        else if (!name)
            name = argv[i];
    }

    if (!name)
    {
        ui_error("download requires a package name");
        cmd_print_usage(USAGE);
        return 1;
    }

    struct repo_list *repos = repo_list_load();
    struct repo_index *idx = NULL;
    const char *found_base_url = NULL;

    for (int i = 0; i < repos->count && !idx; i++)
    {
        idx = api_get_package(repos->urls[i], name);
        if (idx)
            found_base_url = repos->urls[i];
    }

    if (!idx || idx->count == 0)
    {
        ui_errorf("package %s not found in any configured repo", name);
        if (idx)
            repo_index_free(idx);
        repo_list_free(repos);
        return 1;
    }

    const struct repo_package *best = NULL;
    for (size_t i = 0; i < idx->count; i++)
    {
        if (!version || strcmp(idx->items[i].version, version) == 0)
        {
            best = &idx->items[i];
            break;
        }
    }

    if (!best)
    {
        ui_errorf("no build of %s matches version %s", name, version);
        repo_index_free(idx);
        repo_list_free(repos);
        return 1;
    }

    const char *use_arch = arch ? arch : best->architecture;
    const char *use_channel = channel ? channel : best->channel;

    char default_path[512];
    if (!output)
    {
        char *pkgs_dir = path_join(cfg->cache_dir, "pkgs");
        mkdir_p(pkgs_dir);
        char filename[256];
        snprintf(filename, sizeof(filename), "%s-%s-%s.apg", best->name,
                 best->version, use_arch);
        char *joined = path_join(pkgs_dir, filename);
        free(pkgs_dir);
        snprintf(default_path, sizeof(default_path), "%s", joined);
        free(joined);
        output = default_path;
    }

    bool ok = api_download(found_base_url, use_channel, best->name,
                           best->version, use_arch, output, NULL, NULL);

    if (ok)
    {
        ui_successf("downloaded %s to %s", name, output);

        char sig_path[600];
        snprintf(sig_path, sizeof(sig_path), "%s.sig", output);
        if (!api_download_sig(found_base_url, use_channel, best->name,
                              best->version, use_arch, sig_path))
            ui_warn("no detached signature available for this package");
    }
    else
    {
        ui_errorf("failed to download %s", name);
    }

    repo_index_free(idx);
    repo_list_free(repos);

    return ok ? 0 : 1;
}
