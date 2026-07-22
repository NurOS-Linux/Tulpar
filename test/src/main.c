// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>

#include "tests.h"

int
main(void)
{
    test_arg_is_long_form();
    test_arg_is_short_form();
    test_arg_take_value_space();
    test_arg_take_value_equals();
    test_arg_take_value_attached_short();
    test_command_lookup_full_name();
    test_command_lookup_short_alias();
    test_command_lookup_synonyms_share_handler();
    test_command_lookup_unknown();

    test_repo_list_parse_file();
    test_repo_list_parse_file_missing();
    test_repo_list_add_remove();

    test_config_defaults();
    test_config_parse_file_overrides();
    test_config_parse_file_sign_backend();

    test_plan_render_install_step();

    printf("All tulpar tests passed.\n");
    return 0;
}
