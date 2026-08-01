// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "../util/paths.h"

#ifndef TULPAR_SYSCONF_DIR
#define TULPAR_SYSCONF_DIR "/etc/apg"
#endif
#ifndef TULPAR_CACHE_DIR
#define TULPAR_CACHE_DIR "/var/cache/apg"
#endif
#ifndef TULPAR_DB_DIR
#define TULPAR_DB_DIR "/var/lib/apg/db"
#endif
#ifndef TULPAR_LOG_FILE
#define TULPAR_LOG_FILE "/var/log/tulpar.log"
#endif
#ifndef TULPAR_REPODATA_TTL
#define TULPAR_REPODATA_TTL 900
#endif
#ifndef TULPAR_MAX_PARALLEL_DOWNLOADS
#define TULPAR_MAX_PARALLEL_DOWNLOADS 4
#endif

struct tulpar_config *
tulpar_config_defaults(void)
{
    struct tulpar_config *cfg = calloc(1, sizeof(*cfg));
    if (!cfg)
        return NULL;

    cfg->cache_dir = strdup(TULPAR_CACHE_DIR);
    cfg->db_dir = strdup(TULPAR_DB_DIR);
    cfg->log_file = strdup(TULPAR_LOG_FILE);
    cfg->repodata_ttl = TULPAR_REPODATA_TTL;
    cfg->max_parallel_downloads = TULPAR_MAX_PARALLEL_DOWNLOADS;
    cfg->require_signature = false;
    cfg->color_theme = strdup("cyan");
    cfg->verbose = false;
    cfg->quiet = false;
    cfg->journald_mirror = false;

    if (!cfg->cache_dir || !cfg->db_dir || !cfg->log_file || !cfg->color_theme)
    {
        tulpar_config_free(cfg);
        return NULL;
    }

    return cfg;
}

static char *
trim(char *s)
{
    while (isspace((unsigned char)*s))
        s++;
    if (*s == '\0')
        return s;

    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end))
        *end-- = '\0';

    return s;
}

static bool
parse_bool(const char *v)
{
    return strcmp(v, "true") == 0 || strcmp(v, "1") == 0 ||
           strcmp(v, "yes") == 0;
}

static void
apply_kv(struct tulpar_config *cfg, const char *key, const char *value)
{
    if (strcmp(key, "cache_dir") == 0)
    {
        free(cfg->cache_dir);
        cfg->cache_dir = strdup(value);
    }
    else if (strcmp(key, "db_dir") == 0)
    {
        free(cfg->db_dir);
        cfg->db_dir = strdup(value);
    }
    else if (strcmp(key, "log_file") == 0)
    {
        free(cfg->log_file);
        cfg->log_file = strdup(value);
    }
    else if (strcmp(key, "repodata_ttl") == 0)
    {
        cfg->repodata_ttl = atoi(value);
    }
    else if (strcmp(key, "max_parallel_downloads") == 0)
    {
        cfg->max_parallel_downloads = atoi(value);
    }
    else if (strcmp(key, "require_signature") == 0)
    {
        cfg->require_signature = parse_bool(value);
    }
    else if (strcmp(key, "color_theme") == 0)
    {
        free(cfg->color_theme);
        cfg->color_theme = strdup(value);
    }
    else if (strcmp(key, "verbose") == 0)
    {
        cfg->verbose = parse_bool(value);
    }
    else if (strcmp(key, "quiet") == 0)
    {
        cfg->quiet = parse_bool(value);
    }
    else if (strcmp(key, "journald") == 0)
    {
        cfg->journald_mirror = parse_bool(value);
    }
}

bool
tulpar_config_parse_file(const char *path, struct tulpar_config *cfg)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return false;

    char line[512];
    while (fgets(line, sizeof(line), f))
    {
        char *trimmed = trim(line);
        if (trimmed[0] == '\0' || trimmed[0] == '#')
            continue;

        char *eq = strchr(trimmed, '=');
        if (!eq)
            continue;

        *eq = '\0';
        char *key = trim(trimmed);
        char *value = trim(eq + 1);
        if (key[0] == '\0')
            continue;

        apply_kv(cfg, key, value);
    }

    fclose(f);
    return true;
}

struct tulpar_config *
tulpar_config_load(void)
{
    struct tulpar_config *cfg = tulpar_config_defaults();
    if (!cfg)
        return NULL;

    char sys_path[512];
    snprintf(sys_path, sizeof(sys_path), "%s/tulpar.conf", TULPAR_SYSCONF_DIR);
    tulpar_config_parse_file(sys_path, cfg);

    char *user_path = home_config_path("apg/tulpar.conf");
    if (user_path)
    {
        tulpar_config_parse_file(user_path, cfg);
        free(user_path);
    }

    return cfg;
}

void
tulpar_config_free(struct tulpar_config *cfg)
{
    if (!cfg)
        return;

    free(cfg->cache_dir);
    free(cfg->db_dir);
    free(cfg->log_file);
    free(cfg->color_theme);
    free(cfg);
}
