// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>

#include <apg/db.h>
#include <apg/package.h>
#include <apg/sign.h>
#include <apg/transaction.h>

#include "../config/config.h"
#include "../util/paths.h"

void cmd_print_usage(const char *usage);

struct db_handle *cmd_open_db(const struct dest_ctx *dest, bool writable);

void cmd_warn_if_unsigned(const struct package *pkg, sign_backend_t backend);

bool cmd_run_transaction(struct apg_trans *trans, const struct dest_ctx *dest,
                         const struct tulpar_config *cfg, bool assume_yes,
                         bool require_signature_flag,
                         sign_backend_t sign_backend);

bool cmd_parse_sign_backend(const char *value, sign_backend_t *out);
