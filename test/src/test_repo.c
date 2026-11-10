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

static void
free_list(struct repo_list *list)
{
    for (int i = 0; i < list->count; i++)
        free(list->urls[i]);
    free(list->urls);
}

void
test_repo_list_parse_disabled_and_priority(void)
{
    char *path = mktmp_path("repos-prio");
    write_file_contents(path, "# header\n"
                              "https://repo.example.com/late priority=300\n"
                              "#disabled https://repo.example.com/off\n"
                              "https://repo.example.com/default\n"
                              "https://repo.example.com/early priority=5\n"
                              "https://repo.example.com/bad priority=zz\n");

    struct repo_list list = {0};
    assert(repo_list_parse_file(path, &list));
    assert(list.count == 4);
    assert(strcmp(list.urls[0], "https://repo.example.com/early") == 0);
    assert(strcmp(list.urls[1], "https://repo.example.com/default") == 0);
    assert(strcmp(list.urls[2], "https://repo.example.com/bad") == 0);
    assert(strcmp(list.urls[3], "https://repo.example.com/late") == 0);
    free_list(&list);

    struct repo_entries entries = {0};
    assert(repo_entries_parse_file(path, true, &entries));
    assert(entries.count == 5);
    assert(!entries.items[1].enabled);
    assert(entries.items[1].priority == REPO_DEFAULT_PRIORITY);
    assert(entries.items[0].priority == 300);
    assert(entries.items[0].user_file);
    repo_entries_free(&entries);

    unlink(path);
    free(path);
    printf("test_repo_list_parse_disabled_and_priority: PASS\n");
}

void
test_repo_set_enabled_and_priority_preserve_other_lines(void)
{
    char *path = mktmp_path("repos-edit");
    write_file_contents(path, "# keep me\n"
                              "https://repo.example.com/a\n"
                              "\n"
                              "https://repo.example.com/b priority=7\n");

    assert(repo_set_enabled(path, "https://repo.example.com/a", false));
    assert(repo_set_priority(path, "https://repo.example.com/b", 50));
    assert(!repo_set_enabled(path, "https://repo.example.com/zzz", false));
    assert(!repo_set_priority(path, "https://repo.example.com/a", -1));

    struct repo_list list = {0};
    assert(repo_list_parse_file(path, &list));
    assert(list.count == 1);
    assert(strcmp(list.urls[0], "https://repo.example.com/b") == 0);
    free_list(&list);

    assert(repo_set_enabled(path, "https://repo.example.com/a", true));
    assert(repo_set_priority(path, "https://repo.example.com/b",
                             REPO_DEFAULT_PRIORITY));

    char *text = read_file_contents(path);
    assert(text);
    assert(strcmp(text, "# keep me\n"
                        "https://repo.example.com/a\n"
                        "\n"
                        "https://repo.example.com/b\n") == 0);
    free(text);

    assert(repo_set_enabled(path, "https://repo.example.com/a", false));
    assert(repo_list_remove(path, "https://repo.example.com/a"));
    text = read_file_contents(path);
    assert(text);
    assert(strcmp(text, "# keep me\n"
                        "\n"
                        "https://repo.example.com/b\n") == 0);
    free(text);

    unlink(path);
    free(path);
    printf("test_repo_set_enabled_and_priority_preserve_other_lines: PASS\n");
}

void
test_repo_add_reenables_and_long_lines_survive_edits(void)
{
    char *path = mktmp_path("repos-long");
    char long_url[1600];
    memcpy(long_url, "https://repo.example.com/", 25);
    memset(long_url + 25, 'x', sizeof(long_url) - 26);
    long_url[sizeof(long_url) - 1] = '\0';

    char content[2400];
    snprintf(content, sizeof(content),
             "#disabled https://repo.example.com/off\n%s\n", long_url);
    write_file_contents(path, content);

    assert(repo_list_add(path, "https://repo.example.com/off"));
    struct repo_list list = {0};
    assert(repo_list_parse_file(path, &list));
    assert(list.count == 2);
    free_list(&list);

    assert(repo_set_priority(path, long_url, 7));
    assert(repo_set_enabled(path, "https://repo.example.com/off", false));

    struct repo_entries entries = {0};
    assert(repo_entries_parse_file(path, false, &entries));
    assert(entries.count == 2);
    assert(strcmp(entries.items[1].url, long_url) == 0);
    assert(entries.items[1].priority == 7);
    assert(!entries.items[0].enabled);
    repo_entries_free(&entries);

    char tmp_path[600];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
    assert(access(tmp_path, F_OK) != 0);

    unlink(path);
    free(path);
    printf("test_repo_add_reenables_and_long_lines_survive_edits: PASS\n");
}
