// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>

struct repo_list
{
    char **urls;
    int count;
    int cap;
};

struct repo_list *repo_list_new(void);
void repo_list_free(struct repo_list *list);

bool repo_list_parse_file(const char *path, struct repo_list *out);

struct repo_list *repo_list_load(void);

bool repo_list_add(const char *path, const char *url);
bool repo_list_remove(const char *path, const char *url);

char *repo_system_file(void);
char *repo_user_file(void);
