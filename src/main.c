// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli/args.h"
#include "cli/dispatch.h"
#include "cli/ui.h"
#include "config/config.h"
#include "i18n.h"
#include "log/log.h"
#include "net/http.h"

#ifndef TULPAR_VERSION
#define TULPAR_VERSION "0.5.0"
#endif

static void
print_usage(void)
{
    printf(_("usage: tulpar <command> [options] [args]\n\n"));
    printf(_("commands:\n"));

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
               _(g_commands[i].summary));
    }

    printf(_("\nglobal flags:\n"));
    printf(_("  -h, --help              show this help text\n"));
    printf(_("  -V, --version           show the tulpar version\n"));
    printf(_("  -y, --yes               assume yes on confirmation prompts\n"));
    printf(_("  -j, --json              machine-readable output\n"));
    printf(_("  -d, --dest <path>       operate against an alternate root\n"));
    printf(_("  -q, --quiet             suppress informational output\n"));
    printf(_("      --verbose           enable debug output\n"));
    printf(_("      --offline           never touch the network, use cached "
             "indexes\n"));
    printf(_("      --color <when>      colorize output: auto, always, "
             "never\n"));
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
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    textdomain(GETTEXT_PACKAGE);

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
        printf(_("tulpar %s\n"), TULPAR_VERSION);
        return 0;
    }

    const char *token = normalize_command_token(argv[1]);
    const struct command *cmd = command_lookup(token);
    if (!cmd)
    {
        ui_init(NULL, UI_COLOR_AUTO);
        if (argv[1][0] == '-')
            ui_errorf(_("unknown option: %s"), argv[1]);
        else
        {
            ui_errorf(_("unknown command: %s"), argv[1]);
            const char *suggestion = command_suggest(argv[1]);
            if (suggestion)
                ui_hint(_("did you mean '%s'?"), suggestion);
        }
        ui_hint(_("run 'tulpar --help' to see the available commands"));
        return 1;
    }

    bool verbose = false;
    bool quiet = false;
    bool offline = false;
    ui_color_mode color_mode = UI_COLOR_AUTO;

    char **filtered = malloc((size_t)argc * sizeof(char *));
    int filtered_count = 0;

    for (int i = 2; i < argc; i++)
    {
        const char *value = NULL;
        if (strcmp(argv[i], "--") == 0)
        {
            while (i < argc)
                filtered[filtered_count++] = argv[i++];
            break;
        }
        if (arg_take_value(argc, argv, &i, "color", '\0', &value))
        {
            if (!ui_parse_color_mode(value, &color_mode))
            {
                ui_init(NULL, UI_COLOR_AUTO);
                ui_errorf(_("invalid value for --color: %s"), value);
                ui_hint(_("use one of: auto, always, never"));
                free(filtered);
                return 1;
            }
        }
        else if (arg_is(argv[i], "verbose", '\0'))
            verbose = true;
        else if (arg_is(argv[i], "quiet", 'q'))
            quiet = true;
        else if (arg_is(argv[i], "offline", '\0'))
            offline = true;
        else
            filtered[filtered_count++] = argv[i];
    }

    struct tulpar_config *cfg = tulpar_config_load();
    if (!cfg)
    {
        ui_error(_("failed to initialise configuration"));
        free(filtered);
        return 1;
    }

    if (verbose)
        cfg->verbose = true;
    if (quiet)
        cfg->quiet = true;

    ui_init(cfg->color_theme, color_mode);
    ui_set_verbosity(cfg->verbose, cfg->quiet);

    if (log_open(cfg->log_file))
        log_set_verbosity(cfg->verbose, cfg->quiet);
    else
        ui_debug(
            _("could not open the log file, continuing without file logging"));

    log_set_journald_mirror(cfg->journald_mirror);

    http_set_offline(cfg->offline || offline);
    http_global_init();

    int rc = cmd->run(filtered_count, filtered, cfg);

    http_global_cleanup();
    log_close();
    tulpar_config_free(cfg);
    free(filtered);

    return rc;
}
