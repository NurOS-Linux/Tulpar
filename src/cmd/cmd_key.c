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

    for (int i = 0; i < argc; i++)
    {
        if (arg_is_help(argv[i]))
        {
            cmd_print_usage(USAGE);
            return 0;
        }
        else if (!new_key_path)
            new_key_path = argv[i];
        else if (!key_sig_path)
            key_sig_path = argv[i];
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

    if (argc == 0)
    {
        ui_error(_("key requires a sub-action: add, list"));
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
    bool is_list = strcmp(action, "list") == 0 || strcmp(action, "l") == 0;

    if (is_add)
        return run_add(argc - 1, argv + 1);
    if (is_list)
        return run_list();

    ui_errorf(_("unknown key sub-action: %s"), action);
    cmd_print_usage(USAGE);
    return 1;
}
