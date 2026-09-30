// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>
#include <stddef.h>

struct download_task
{
    const char *url;
    const char *dest_path;
    const char *label;
    bool optional;
    bool ok;
    char error[512];
    size_t now;
    size_t total;
};

bool download_all_parallel(struct download_task *tasks, size_t count,
                           int max_parallel);
