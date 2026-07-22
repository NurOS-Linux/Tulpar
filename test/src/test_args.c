// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>

#include "cli/args.h"
#include "cli/dispatch.h"
#include "helpers.h"
#include "tests.h"

void
test_arg_is_long_form(void)
{
    assert(arg_is("--yes", "yes", 'y'));
    assert(!arg_is("--no", "yes", 'y'));
    assert(!arg_is("yes", "yes", 'y'));
    printf("test_arg_is_long_form: PASS\n");
}

void
test_arg_is_short_form(void)
{
    assert(arg_is("-y", "yes", 'y'));
    assert(!arg_is("-x", "yes", 'y'));
    assert(!arg_is("-yy", "yes", 'y'));
    printf("test_arg_is_short_form: PASS\n");
}

void
test_arg_take_value_space(void)
{
    char *argv[] = {"--dest", "/mnt/root"};
    int i = 0;
    const char *value = NULL;
    assert(arg_take_value(2, argv, &i, "dest", 'd', &value));
    assert(strcmp(value, "/mnt/root") == 0);
    assert(i == 1);
    printf("test_arg_take_value_space: PASS\n");
}

void
test_arg_take_value_equals(void)
{
    char *argv[] = {"--dest=/mnt/root"};
    int i = 0;
    const char *value = NULL;
    assert(arg_take_value(1, argv, &i, "dest", 'd', &value));
    assert(strcmp(value, "/mnt/root") == 0);
    assert(i == 0);
    printf("test_arg_take_value_equals: PASS\n");
}

void
test_arg_take_value_attached_short(void)
{
    char *argv[] = {"-d/mnt/root"};
    int i = 0;
    const char *value = NULL;
    assert(arg_take_value(1, argv, &i, "dest", 'd', &value));
    assert(strcmp(value, "/mnt/root") == 0);
    printf("test_arg_take_value_attached_short: PASS\n");
}

void
test_command_lookup_full_name(void)
{
    const struct command *cmd = command_lookup("install");
    assert(cmd);
    assert(strcmp(cmd->name, "install") == 0);
    printf("test_command_lookup_full_name: PASS\n");
}

void
test_command_lookup_short_alias(void)
{
    const struct command *cmd = command_lookup("i");
    assert(cmd);
    assert(strcmp(cmd->name, "install") == 0);
    assert(cmd->alias == 'i');
    printf("test_command_lookup_short_alias: PASS\n");
}

void
test_command_lookup_synonyms_share_handler(void)
{
    const struct command *remove_cmd = command_lookup("remove");
    const struct command *uninstall_cmd = command_lookup("uninstall");
    assert(remove_cmd && uninstall_cmd);
    assert(remove_cmd->run == uninstall_cmd->run);
    assert(remove_cmd->alias == uninstall_cmd->alias);
    printf("test_command_lookup_synonyms_share_handler: PASS\n");
}

void
test_command_lookup_unknown(void)
{
    assert(command_lookup("nonexistent") == NULL);
    assert(command_lookup("") == NULL);
    printf("test_command_lookup_unknown: PASS\n");
}
