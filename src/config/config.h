// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>

struct tulpar_config
{
    char *cache_dir;
    char *db_dir;
    char *log_file;
    int repodata_ttl;
    int max_parallel_downloads;
    bool require_signature;
    char *color_theme;
    bool verbose;
    bool quiet;
    bool journald_mirror;
};

struct tulpar_config *tulpar_config_defaults(void);

struct tulpar_config *tulpar_config_load(void);

bool tulpar_config_parse_file(const char *path, struct tulpar_config *cfg);

void tulpar_config_free(struct tulpar_config *cfg);
