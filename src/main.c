// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli/args.h"
#include "cli/dispatch.h"
#include "cli/ui.h"
#include "config/config.h"
#include "log/log.h"
#include "net/http.h"

#ifndef TULPAR_VERSION
#define TULPAR_VERSION "0.1.0"
#endif

static void
print_usage(void)
{
    printf("usage: tulpar <command> [options] [args]\n\n");
    printf("commands:\n");

    for (size_t i = 0; i < g_command_count; i++)
    {
        bool seen = false;
        for (size_t j = 0; j < i; j++)
        {
            if (g_commands[j].alias == g_commands[i].alias)
            {
                seen = true;
                break;
            }
        }
        if (seen)
            continue;
        printf("  %-12s -%-2c %s\n", g_commands[i].name, g_commands[i].alias,
               g_commands[i].summary);
    }

    printf("\nglobal flags:\n");
    printf("  -h, --help              show this help text\n");
    printf("  -V, --version           show the tulpar version\n");
    printf("  -y, --yes               assume yes on confirmation prompts\n");
    printf("  -j, --json              machine-readable output\n");
    printf("  -d, --dest <path>       operate against an alternate root\n");
    printf("  -q, --quiet             suppress informational output\n");
    printf("      --verbose           enable debug output\n");
}

static const char *
normalize_command_token(const char *arg)
{
    if (arg[0] != '-')
        return arg;

    if (arg[1] != '\0' && arg[2] == '\0')
        return arg + 1;

    return NULL;
}

int
main(int argc, char **argv)
{
    if (argc < 2)
    {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
    {
        print_usage();
        return 0;
    }

    if (strcmp(argv[1], "-V") == 0 || strcmp(argv[1], "--version") == 0)
    {
        printf("tulpar %s\n", TULPAR_VERSION);
        return 0;
    }

    const char *token = normalize_command_token(argv[1]);
    const struct command *cmd = command_lookup(token);
    if (!cmd)
    {
        ui_error("unknown command");
        print_usage();
        return 1;
    }

    bool verbose = false;
    bool quiet = false;

    char **filtered = malloc((size_t)argc * sizeof(char *));
    int filtered_count = 0;

    for (int i = 2; i < argc; i++)
    {
        if (arg_is(argv[i], "verbose", '\0'))
            verbose = true;
        else if (arg_is(argv[i], "quiet", 'q'))
            quiet = true;
        else
            filtered[filtered_count++] = argv[i];
    }

    struct tulpar_config *cfg = tulpar_config_load();
    if (!cfg)
    {
        ui_error("failed to initialise configuration");
        free(filtered);
        return 1;
    }

    if (verbose)
        cfg->verbose = true;
    if (quiet)
        cfg->quiet = true;

    ui_init(cfg->color_theme);
    ui_set_verbosity(cfg->verbose, cfg->quiet);

    if (log_open(cfg->log_file))
        log_set_verbosity(cfg->verbose, cfg->quiet);
    else
        ui_debug(
            "could not open the log file, continuing without file logging");

    log_set_journald_mirror(cfg->journald_mirror);

    http_global_init();

    int rc = cmd->run(filtered_count, filtered, cfg);

    http_global_cleanup();
    log_close();
    tulpar_config_free(cfg);
    free(filtered);

    return rc;
}
