// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <string.h>

#include <apg/keyring.h>

#include "cmd_key.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"

#ifndef TULPAR_KEYRING_DIR
#define TULPAR_KEYRING_DIR "/etc/apg/trusted.d"
#endif

#define USAGE                                                                  \
    "tulpar key add <key-path> [sig-path] [--sign-backend sodium|gpgme]\n"     \
    "       tulpar key list [--sign-backend sodium|gpgme]"

static int
run_add_sodium(const char *new_key_path, const char *key_sig_path)
{
    struct keyring *trusted = keyring_load(TULPAR_KEYRING_DIR);
    if (!trusted)
    {
        ui_errorf("failed to load trusted keyring from %s", TULPAR_KEYRING_DIR);
        return 1;
    }

    bool ok = keyring_add_key(TULPAR_KEYRING_DIR, new_key_path, key_sig_path,
                              trusted);
    keyring_free(trusted);
    return ok ? 0 : 1;
}

static int
run_add_gpgme(const char *new_key_path, const char *key_sig_path)
{
    struct keyring_gpgme *trusted = keyring_load_gpgme(TULPAR_KEYRING_DIR);
    if (!trusted)
    {
        ui_errorf("failed to load trusted keyring from %s", TULPAR_KEYRING_DIR);
        return 1;
    }

    bool ok = keyring_add_key_gpgme(TULPAR_KEYRING_DIR, new_key_path,
                                    key_sig_path, trusted);
    keyring_free_gpgme(trusted);
    return ok ? 0 : 1;
}

static int
run_add(int argc, char **argv, sign_backend_t backend)
{
    const char *new_key_path = NULL;
    const char *key_sig_path = NULL;

    for (int i = 0; i < argc; i++)
    {
        const char *value = NULL;
        if (arg_is_help(argv[i]))
        {
            cmd_print_usage(USAGE);
            return 0;
        }
        else if (arg_take_value(argc, argv, &i, "sign-backend", '\0', &value))
        {
            if (!cmd_parse_sign_backend(value, &backend))
            {
                ui_error("--sign-backend must be sodium or gpgme");
                cmd_print_usage(USAGE);
                return 1;
            }
        }
        else if (!new_key_path)
            new_key_path = argv[i];
        else if (!key_sig_path)
            key_sig_path = argv[i];
    }

    if (!new_key_path)
    {
        ui_error("key add requires a path to the new public key");
        cmd_print_usage(USAGE);
        return 1;
    }

    int rc = backend == SIGN_BACKEND_GPGME
                 ? run_add_gpgme(new_key_path, key_sig_path)
                 : run_add_sodium(new_key_path, key_sig_path);

    if (rc == 0)
        ui_success("key added to the trusted keyring");
    else
        ui_error("key was not added; it must be endorsed by an already "
                 "trusted key");

    return rc;
}

static int
run_list(sign_backend_t backend)
{
    printf("Trusted keyring directory: %s\n", TULPAR_KEYRING_DIR);
    printf("Backend: %s\n", backend == SIGN_BACKEND_GPGME ? "gpgme" : "sodium");
    ui_info("use your platform's file listing to inspect individual keys");
    return 0;
}

int
cmd_key_run(int argc, char **argv, struct tulpar_config *cfg)
{
    if (argc == 0)
    {
        ui_error("key requires a sub-action: add, list");
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
        return run_add(argc - 1, argv + 1, cfg->sign_backend);
    if (is_list)
        return run_list(cfg->sign_backend);

    ui_errorf("unknown key sub-action: %s", action);
    cmd_print_usage(USAGE);
    return 1;
}
