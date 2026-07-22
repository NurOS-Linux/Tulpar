// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>

typedef enum
{
    TULPAR_LOG_DEBUG,
    TULPAR_LOG_INFO,
    TULPAR_LOG_WARN,
    TULPAR_LOG_ERROR,
} log_level_t;

bool log_open(const char *path);
void log_close(void);

void log_set_verbosity(bool verbose, bool quiet);
void log_set_journald_mirror(bool enabled);

void log_write(log_level_t level, const char *msg);
