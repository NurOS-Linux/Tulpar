// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>
#include <stddef.h>

struct repo_package
{
    char *name;
    char *version;
    char *architecture;
    char *channel;
    char *type;
    char *description;
};

struct repo_index
{
    struct repo_package *items;
    size_t count;
};

struct repo_index *repo_index_parse_json(const char *json, size_t len);
void repo_index_free(struct repo_index *idx);

char *repodata_cache_file(const char *cache_dir, const char *repo_url);

bool repodata_is_fresh(const char *cache_file, int ttl_seconds);

bool repodata_refresh(const char *repo_url, const char *cache_dir);

struct repo_index *repodata_load(const char *repo_url, const char *cache_dir,
                                 int ttl_seconds, bool force_refresh);

struct repo_index *repodata_read_cache_file(const char *cache_file);
