// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "repo.h"
#include "../util/paths.h"

#ifndef TULPAR_SYSCONF_DIR
#define TULPAR_SYSCONF_DIR "/etc/apg"
#endif

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

struct repo_list *
repo_list_new(void)
{
    struct repo_list *list = calloc(1, sizeof(*list));
    return list;
}

static bool
repo_list_push(struct repo_list *list, const char *url)
{
    if (list->count == list->cap)
    {
        int new_cap = list->cap == 0 ? 8 : list->cap * 2;
        char **tmp = realloc(list->urls, (size_t)new_cap * sizeof(char *));
        if (!tmp)
            return false;
        list->urls = tmp;
        list->cap = new_cap;
    }

    char *dup = strdup(url);
    if (!dup)
        return false;

    list->urls[list->count++] = dup;
    return true;
}

bool
repo_list_parse_file(const char *path, struct repo_list *out)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return false;

    char line[1024];
    while (fgets(line, sizeof(line), f))
    {
        char *trimmed = trim(line);
        if (trimmed[0] == '\0' || trimmed[0] == '#')
            continue;
        repo_list_push(out, trimmed);
    }

    fclose(f);
    return true;
}

struct repo_list *
repo_list_load(void)
{
    struct repo_list *list = repo_list_new();
    if (!list)
        return NULL;

    char *sys_file = repo_system_file();
    if (sys_file)
    {
        repo_list_parse_file(sys_file, list);
        free(sys_file);
    }

    char *user_file = repo_user_file();
    if (user_file)
    {
        repo_list_parse_file(user_file, list);
        free(user_file);
    }

    return list;
}

void
repo_list_free(struct repo_list *list)
{
    if (!list)
        return;

    for (int i = 0; i < list->count; i++)
        free(list->urls[i]);
    free(list->urls);
    free(list);
}

static bool
repo_list_contains(const char *path, const char *url)
{
    struct repo_list tmp = {0};
    if (!repo_list_parse_file(path, &tmp))
        return false;

    bool found = false;
    for (int i = 0; i < tmp.count; i++)
    {
        if (strcmp(tmp.urls[i], url) == 0)
        {
            found = true;
            break;
        }
    }

    for (int i = 0; i < tmp.count; i++)
        free(tmp.urls[i]);
    free(tmp.urls);
    return found;
}

static char *
dirname_of(const char *path)
{
    const char *slash = strrchr(path, '/');
    if (!slash)
        return strdup(".");
    size_t len = (size_t)(slash - path);
    if (len == 0)
        return strdup("/");
    char *dir = malloc(len + 1);
    if (!dir)
        return NULL;
    memcpy(dir, path, len);
    dir[len] = '\0';
    return dir;
}

bool
repo_list_add(const char *path, const char *url)
{
    if (repo_list_contains(path, url))
        return true;

    char *dir = dirname_of(path);
    if (dir)
    {
        mkdir_p(dir);
        free(dir);
    }

    FILE *f = fopen(path, "a");
    if (!f)
        return false;

    fprintf(f, "%s\n", url);
    fclose(f);
    return true;
}

bool
repo_list_remove(const char *path, const char *url)
{
    struct repo_list tmp = {0};
    if (!repo_list_parse_file(path, &tmp))
        return false;

    FILE *f = fopen(path, "w");
    if (!f)
    {
        for (int i = 0; i < tmp.count; i++)
            free(tmp.urls[i]);
        free(tmp.urls);
        return false;
    }

    bool removed = false;
    for (int i = 0; i < tmp.count; i++)
    {
        if (strcmp(tmp.urls[i], url) == 0)
        {
            removed = true;
            continue;
        }
        fprintf(f, "%s\n", tmp.urls[i]);
    }

    fclose(f);

    for (int i = 0; i < tmp.count; i++)
        free(tmp.urls[i]);
    free(tmp.urls);

    return removed;
}

char *
repo_system_file(void)
{
    return path_join(TULPAR_SYSCONF_DIR, "repos");
}

char *
repo_user_file(void)
{
    return home_config_path("apg/repos");
}
