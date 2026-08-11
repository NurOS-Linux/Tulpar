// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <apg/transaction.h>

#if defined(__GNUC__) || defined(__clang__)
#define TULPAR_PRINTF(fmt_idx, first_idx)                                      \
    __attribute__((format(printf, fmt_idx, first_idx)))
#else
#define TULPAR_PRINTF(fmt_idx, first_idx)
#endif

void ui_init(const char *color_theme);

bool ui_colors_enabled(void);

void ui_set_verbosity(bool verbose, bool quiet);

void ui_success(const char *msg);
void ui_error(const char *msg);
void ui_warn(const char *msg);
void ui_info(const char *msg);
void ui_debug(const char *msg);

void ui_successf(const char *fmt, ...) TULPAR_PRINTF(1, 2);
void ui_errorf(const char *fmt, ...) TULPAR_PRINTF(1, 2);
void ui_warnf(const char *fmt, ...) TULPAR_PRINTF(1, 2);
void ui_infof(const char *fmt, ...) TULPAR_PRINTF(1, 2);
void ui_debugf(const char *fmt, ...) TULPAR_PRINTF(1, 2);

bool ui_confirm(const char *prompt, bool assume_yes);

int ui_select(const char *prompt, const char *const *options, int count,
              bool assume_yes);

void ui_progress_start(const char *label, size_t total);
void ui_progress_update(size_t current);
void ui_progress_finish(void);

void ui_print_plan(const struct apg_trans *trans);
void ui_print_conflicts(const struct apg_trans *trans);
