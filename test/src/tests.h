// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

void test_arg_is_long_form(void);
void test_arg_is_short_form(void);
void test_arg_take_value_space(void);
void test_arg_take_value_equals(void);
void test_arg_take_value_attached_short(void);
void test_command_lookup_full_name(void);
void test_command_lookup_short_alias(void);
void test_command_lookup_synonyms_share_handler(void);
void test_command_lookup_unknown(void);

void test_repo_list_parse_file(void);
void test_repo_list_parse_file_missing(void);
void test_repo_list_add_remove(void);
void test_repo_list_parse_disabled_and_priority(void);
void test_repo_set_enabled_and_priority_preserve_other_lines(void);

void test_config_defaults(void);
void test_config_parse_file_overrides(void);

void test_plan_render_install_step(void);

void test_offline_http_refuses_requests(void);
void test_offline_repodata_uses_stale_cache(void);

void test_since_spec_relative_units(void);
void test_since_spec_dates_and_epoch(void);
void test_since_spec_rejects_malformed_input(void);

void test_why_chain_follows_dependents_to_explicit_package(void);

void test_json_out_op_and_status_names(void);
void test_json_out_package_list_schema(void);
void test_json_out_package_info_schema(void);
void test_json_out_repos_schema(void);
void test_json_out_stats_schema(void);
void test_json_out_files_schema(void);
void test_json_out_owners_schema(void);
void test_json_out_history_and_search_schema(void);
