// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <unistd.h>

#include "helpers.h"
#include "repo/repo.h"
#include "tests.h"

void
test_repo_list_parse_file(void)
{
    char *path = mktmp_path("repos");
    write_file_contents(path, "# a comment\n"
                              "https://repo.example.com/one\n"
                              "\n"
                              "https://repo.example.com/two\n");

    struct repo_list list = {0};
    assert(repo_list_parse_file(path, &list));
    assert(list.count == 2);
    assert(strcmp(list.urls[0], "https://repo.example.com/one") == 0);
    assert(strcmp(list.urls[1], "https://repo.example.com/two") == 0);

    for (int i = 0; i < list.count; i++)
        free(list.urls[i]);
    free(list.urls);
    unlink(path);
    free(path);
    printf("test_repo_list_parse_file: PASS\n");
}

void
test_repo_list_parse_file_missing(void)
{
    struct repo_list list = {0};
    assert(!repo_list_parse_file("/nonexistent/path/repos", &list));
    assert(list.count == 0);
    printf("test_repo_list_parse_file_missing: PASS\n");
}

void
test_repo_list_add_remove(void)
{
    char *path = mktmp_path("repos-rw");
    unlink(path);

    assert(repo_list_add(path, "https://repo.example.com/a"));
    assert(repo_list_add(path, "https://repo.example.com/b"));
    assert(repo_list_add(path, "https://repo.example.com/a"));

    struct repo_list list = {0};
    assert(repo_list_parse_file(path, &list));
    assert(list.count == 2);
    for (int i = 0; i < list.count; i++)
        free(list.urls[i]);
    free(list.urls);

    assert(repo_list_remove(path, "https://repo.example.com/a"));

    struct repo_list after = {0};
    assert(repo_list_parse_file(path, &after));
    assert(after.count == 1);
    assert(strcmp(after.urls[0], "https://repo.example.com/b") == 0);
    for (int i = 0; i < after.count; i++)
        free(after.urls[i]);
    free(after.urls);

    unlink(path);
    free(path);
    printf("test_repo_list_add_remove: PASS\n");
}
