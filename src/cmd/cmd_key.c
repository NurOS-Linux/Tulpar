// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <string.h>

#include <apg/keyring.h>

#include "cmd_key.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../i18n.h"

#ifndef TULPAR_KEYRING_DIR
#define TULPAR_KEYRING_DIR "/etc/apg/trusted.d"
#endif

#define USAGE                                                                  \
    "tulpar key add <key-path> [sig-path]\n"                                   \
    "       tulpar key list"

static int
run_add(int argc, char **argv)
{
    const char *new_key_path = NULL;
    const char *key_sig_path = NULL;
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
        else if (end_of_options || argv[i][0] != '-')
        {
            if (!new_key_path)
                new_key_path = argv[i];
            else if (!key_sig_path)
                key_sig_path = argv[i];
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

    if (!new_key_path)
    {
        ui_error(_("key add requires a path to the new public key"));
        cmd_print_usage(USAGE);
        return 1;
    }

    struct keyring *trusted = keyring_load(TULPAR_KEYRING_DIR);
    if (!trusted)
    {
        ui_errorf(_("failed to load trusted keyring from %s"),
                  TULPAR_KEYRING_DIR);
        return 1;
    }

    bool ok = keyring_add_key(TULPAR_KEYRING_DIR, new_key_path, key_sig_path,
                              trusted);
    keyring_free(trusted);

    if (ok)
        ui_success(_("key added to the trusted keyring"));
    else
        ui_error(_("key was not added; it must be endorsed by an already "
                   "trusted key"));

    return ok ? 0 : 1;
}

static int
run_list(void)
{
    printf("Trusted keyring directory: %s\n", TULPAR_KEYRING_DIR);
    ui_info(_("use your platform's file listing to inspect individual keys"));
    return 0;
}

int
cmd_key_run(int argc, char **argv, struct tulpar_config *cfg)
{
    (void)cfg;
    if (argc < 1 || arg_is_help(argv[0]))
    {
        cmd_print_usage(USAGE);
        return 0;
    }

    if (strcmp(argv[0], "add") == 0)
        return run_add(argc - 1, argv + 1);
    if (strcmp(argv[0], "list") == 0)
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
                ui_errorf(_("unknown option: %s"), argv[i]);
                cmd_print_usage(USAGE);
                return 1;
            }
        }
        return run_list();
    }

    ui_error(_("unknown key subcommand; expected add or list"));
    cmd_print_usage(USAGE);
    return 1;
}
