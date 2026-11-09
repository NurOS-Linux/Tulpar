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

static bool
entries_push(struct repo_entries *entries, const char *url, bool enabled,
             int priority, bool user_file)
{
    if (entries->count == entries->cap)
    {
        int new_cap = entries->cap == 0 ? 8 : entries->cap * 2;
        struct repo_entry *tmp =
            realloc(entries->items, (size_t)new_cap * sizeof(*tmp));
        if (!tmp)
            return false;
        entries->items = tmp;
        entries->cap = new_cap;
    }

    char *dup = strdup(url);
    if (!dup)
        return false;

    struct repo_entry *e = &entries->items[entries->count++];
    e->url = dup;
    e->enabled = enabled;
    e->priority = priority;
    e->user_file = user_file;
    return true;
}

void
repo_entries_free(struct repo_entries *entries)
{
    if (!entries)
        return;
    for (int i = 0; i < entries->count; i++)
        free(entries->items[i].url);
    free(entries->items);
    entries->items = NULL;
    entries->count = 0;
    entries->cap = 0;
}

static bool
starts_with_marker(const char *line)
{
    static const char marker[] = "#disabled";
    size_t len = sizeof(marker) - 1;
    return strncmp(line, marker, len) == 0 &&
           (line[len] == ' ' || line[len] == '\t');
}

static bool
parse_line(char *line, char **url, bool *enabled, int *priority)
{
    char *trimmed = trim(line);
    *enabled = true;
    *priority = REPO_DEFAULT_PRIORITY;

    if (starts_with_marker(trimmed))
    {
        *enabled = false;
        trimmed = trim(trimmed + strlen("#disabled"));
    }
    if (trimmed[0] == '\0' || trimmed[0] == '#')
        return false;

    *url = trimmed;
    char *sep = trimmed;
    while (*sep && !isspace((unsigned char)*sep))
        sep++;
    if (*sep == '\0')
        return true;
    *sep++ = '\0';

    char *save = NULL;
    for (char *tok = strtok_r(sep, " \t", &save); tok;
         tok = strtok_r(NULL, " \t", &save))
    {
        if (strncmp(tok, "priority=", 9) == 0)
        {
            char *end = NULL;
            long value = strtol(tok + 9, &end, 10);
            if (end != tok + 9 && *end == '\0' && value >= 0 && value <= 100000)
                *priority = (int)value;
        }
    }
    return true;
}

bool
repo_entries_parse_file(const char *path, bool user_file,
                        struct repo_entries *out)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return false;

    char line[1024];
    while (fgets(line, sizeof(line), f))
    {
        char *url = NULL;
        bool enabled = true;
        int priority = REPO_DEFAULT_PRIORITY;
        if (parse_line(line, &url, &enabled, &priority))
            entries_push(out, url, enabled, priority, user_file);
    }

    fclose(f);
    return true;
}

static int
compare_priority(const void *a, const void *b)
{
    const struct repo_entry *ea = a;
    const struct repo_entry *eb = b;
    if (ea->priority != eb->priority)
        return ea->priority < eb->priority ? -1 : 1;
    return 0;
}

static void
entries_sort(struct repo_entries *entries)
{
    for (int i = 1; i < entries->count; i++)
    {
        struct repo_entry cur = entries->items[i];
        int j = i - 1;
        while (j >= 0 && compare_priority(&entries->items[j], &cur) > 0)
        {
            entries->items[j + 1] = entries->items[j];
            j--;
        }
        entries->items[j + 1] = cur;
    }
}

bool
repo_entries_load(struct repo_entries *out)
{
    char *sys_file = repo_system_file();
    if (sys_file)
    {
        repo_entries_parse_file(sys_file, false, out);
        free(sys_file);
    }

    char *user_file = repo_user_file();
    if (user_file)
    {
        repo_entries_parse_file(user_file, true, out);
        free(user_file);
    }

    entries_sort(out);
    return true;
}

static bool
list_from_entries(struct repo_entries *entries, struct repo_list *out)
{
    entries_sort(entries);
    for (int i = 0; i < entries->count; i++)
        if (entries->items[i].enabled)
            repo_list_push(out, entries->items[i].url);
    return true;
}

bool
repo_list_parse_file(const char *path, struct repo_list *out)
{
    struct repo_entries entries = {0};
    if (!repo_entries_parse_file(path, false, &entries))
        return false;
    list_from_entries(&entries, out);
    repo_entries_free(&entries);
    return true;
}

struct repo_list *
repo_list_load(void)
{
    struct repo_list *list = repo_list_new();
    if (!list)
        return NULL;

    struct repo_entries entries = {0};
    repo_entries_load(&entries);
    list_from_entries(&entries, list);
    repo_entries_free(&entries);

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
    struct repo_entries entries = {0};
    if (!repo_entries_parse_file(path, false, &entries))
        return false;

    bool found = false;
    for (int i = 0; i < entries.count && !found; i++)
        found = strcmp(entries.items[i].url, url) == 0;

    repo_entries_free(&entries);
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

enum edit_op
{
    EDIT_REMOVE,
    EDIT_SET_ENABLED,
    EDIT_SET_PRIORITY,
};

static bool
edit_file(const char *path, const char *url, enum edit_op op, int value)
{
    FILE *in = fopen(path, "r");
    if (!in)
        return false;

    size_t cap = 4096;
    size_t len = 0;
    char *out = malloc(cap);
    if (!out)
    {
        fclose(in);
        return false;
    }
    out[0] = '\0';

    bool matched = false;
    char line[1024];
    while (fgets(line, sizeof(line), in))
    {
        char copy[1024];
        snprintf(copy, sizeof(copy), "%s", line);

        char *line_url = NULL;
        bool enabled = true;
        int priority = REPO_DEFAULT_PRIORITY;
        char rebuilt[2200];
        const char *emit = line;

        if (parse_line(copy, &line_url, &enabled, &priority) &&
            strcmp(line_url, url) == 0)
        {
            matched = true;
            if (op == EDIT_REMOVE)
                emit = NULL;
            else
            {
                if (op == EDIT_SET_ENABLED)
                    enabled = value != 0;
                else
                    priority = value;

                int n = snprintf(rebuilt, sizeof(rebuilt), "%s%s",
                                 enabled ? "" : "#disabled ", line_url);
                if (priority != REPO_DEFAULT_PRIORITY)
                    n += snprintf(rebuilt + n, sizeof(rebuilt) - (size_t)n,
                                  " priority=%d", priority);
                snprintf(rebuilt + n, sizeof(rebuilt) - (size_t)n, "\n");
                emit = rebuilt;
            }
        }

        if (!emit)
            continue;
        size_t emit_len = strlen(emit);
        if (len + emit_len + 1 > cap)
        {
            while (len + emit_len + 1 > cap)
                cap *= 2;
            char *tmp = realloc(out, cap);
            if (!tmp)
            {
                free(out);
                fclose(in);
                return false;
            }
            out = tmp;
        }
        memcpy(out + len, emit, emit_len + 1);
        len += emit_len;
    }
    fclose(in);

    if (!matched)
    {
        free(out);
        return false;
    }

    FILE *f = fopen(path, "w");
    bool ok = f && fwrite(out, 1, len, f) == len;
    if (f && fclose(f) != 0)
        ok = false;
    free(out);
    return ok;
}

bool
repo_list_remove(const char *path, const char *url)
{
    return edit_file(path, url, EDIT_REMOVE, 0);
}

bool
repo_set_enabled(const char *path, const char *url, bool enabled)
{
    return edit_file(path, url, EDIT_SET_ENABLED, enabled ? 1 : 0);
}

bool
repo_set_priority(const char *path, const char *url, int priority)
{
    if (priority < 0 || priority > 100000)
        return false;
    return edit_file(path, url, EDIT_SET_PRIORITY, priority);
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
