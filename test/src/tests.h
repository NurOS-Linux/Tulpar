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

void test_config_defaults(void);
void test_config_parse_file_overrides(void);
void test_config_parse_file_sign_backend(void);

void test_plan_render_install_step(void);
