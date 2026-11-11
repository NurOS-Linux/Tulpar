// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include <apg/db.h>
#include <apg/package.h>
#include <apg/transaction.h>

#include "../config/config.h"
#include "../util/paths.h"

void cmd_print_usage(const char *usage);

void cmd_unknown_option(const char *arg, const char *usage);

bool cmd_option_takes_value(const char *arg, const char *usage);

const char *cmd_libapg_reason(void);

struct package *cmd_parse_package(const char *path);

void cmd_warn_repo_unreachable(const char *repo_url, bool using_cache);

void cmd_download_progress(size_t now, size_t total, void *userdata);

void cmd_report_downloaded(const char *what, const char *path);

double cmd_seconds_since(const struct timespec *start);

struct db_handle *cmd_open_db(const struct dest_ctx *dest, bool writable);

void cmd_warn_unsigned(struct package *const *pkgs, size_t count,
                       bool required);

bool cmd_run_transaction(struct apg_trans *trans, struct db_handle *db,
                         const struct dest_ctx *dest,
                         const struct tulpar_config *cfg, bool assume_yes,
                         bool require_signature_flag, bool nodeps,
                         bool skip_dependents, bool dry_run);
