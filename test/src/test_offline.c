// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "helpers.h"
#include "net/http.h"
#include "repo/repodata.h"
#include "tests.h"

void
test_offline_http_refuses_requests(void)
{
    assert(!http_is_offline());
    http_set_offline(true);
    assert(http_is_offline());

    struct http_response resp = {0};
    assert(!http_get("http://127.0.0.1:9/never", &resp));
    assert(strstr(http_last_error(), "offline") != NULL);
    assert(resp.body == NULL);

    char *dest = mktmp_path("offline-dl");
    assert(!http_download("http://127.0.0.1:9/never", dest, NULL, NULL, &resp));
    assert(strstr(http_last_error(), "offline") != NULL);
    unlink(dest);
    free(dest);

    http_set_offline(false);
    assert(!http_is_offline());
    printf("test_offline_http_refuses_requests: PASS\n");
}

void
test_offline_repodata_uses_stale_cache(void)
{
    char *cache = mktmp_dir("offline-cache");
    const char *url = "http://127.0.0.1:9/repo";
    char *file = repodata_cache_file(cache, url);
    assert(file);

    char *slash = strrchr(file, '/');
    *slash = '\0';
    mkdir(file, 0755);
    *slash = '/';
    write_file_contents(file, "[{\"name\": \"cached\", \"version\": \"1.0\"}]");

    http_set_offline(true);
    bool refresh_failed = true;
    struct repo_index *idx =
        repodata_load(url, cache, 0, true, &refresh_failed);
    http_set_offline(false);

    assert(idx);
    assert(!refresh_failed);
    assert(idx->count == 1);
    assert(strcmp(idx->items[0].name, "cached") == 0);
    repo_index_free(idx);

    unlink(file);
    free(file);
    char cmd[300];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", cache);
    assert(system(cmd) == 0);
    free(cache);
    printf("test_offline_repodata_uses_stale_cache: PASS\n");
}
