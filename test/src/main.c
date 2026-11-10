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
    test_repo_list_parse_disabled_and_priority();
    test_repo_set_enabled_and_priority_preserve_other_lines();
    test_repo_add_reenables_and_long_lines_survive_edits();

    test_config_defaults();
    test_config_parse_file_overrides();

    test_plan_render_install_step();

    test_cmd_mark_switches_installed_by_hand();
    test_cmd_owns_and_files_report_the_database();
    test_cmd_reinstall_restores_files_from_an_archive();

    test_offline_http_refuses_requests();
    test_offline_repodata_uses_stale_cache();

    test_since_spec_relative_units();
    test_since_spec_dates_and_epoch();
    test_since_spec_rejects_malformed_input();

    test_why_chain_follows_dependents_to_explicit_package();

    test_json_out_op_and_status_names();
    test_json_out_package_list_schema();
    test_json_out_package_info_schema();
    test_json_out_repos_schema();
    test_json_out_stats_schema();
    test_json_out_files_schema();
    test_json_out_owners_schema();
    test_json_out_history_and_search_schema();

    printf("All tulpar tests passed.\n");
    return 0;
}
