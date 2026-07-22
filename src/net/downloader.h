// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>
#include <stddef.h>

struct download_task
{
    char *url;
    char *dest_path;
    bool ok;
};

void download_all_parallel(struct download_task *tasks, size_t count,
                           int max_parallel);
