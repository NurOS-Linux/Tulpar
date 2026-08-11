// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <yyjson.h>

#include "repodata.h"
#include "../net/http.h"
#include "../util/paths.h"

static unsigned int
fnv1a_hash(const char *s)
{
    unsigned int hash = 2166136261u;
    while (*s)
    {
        hash ^= (unsigned char)*s++;
        hash *= 16777619u;
    }
    return hash;
}

static char *
dup_str_or_empty(yyjson_val *obj, const char *key)
{
    yyjson_val *v = yyjson_obj_get(obj, key);
    const char *s = yyjson_is_str(v) ? yyjson_get_str(v) : "";
    return strdup(s);
}

static char **
dup_str_array(yyjson_val *obj, const char *key, size_t *out_count)
{
    yyjson_val *arr = yyjson_obj_get(obj, key);
    *out_count = 0;
    if (!yyjson_is_arr(arr))
        return NULL;

    size_t count = yyjson_arr_size(arr);
    if (count == 0)
        return NULL;

    char **items = calloc(count, sizeof(char *));
    if (!items)
        return NULL;

    size_t idx, max;
    yyjson_val *val;
    yyjson_arr_foreach(arr, idx, max, val) items[idx] =
        strdup(yyjson_is_str(val) ? yyjson_get_str(val) : "");

    *out_count = count;
    return items;
}

struct repo_index *
repo_index_parse_json(const char *json, size_t len)
{
    yyjson_doc *doc = yyjson_read(json, len, 0);
    if (!doc)
        return NULL;

    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *arr = root;
    if (yyjson_is_obj(root))
    {
        yyjson_val *packages = yyjson_obj_get(root, "packages");
        if (yyjson_is_arr(packages))
            arr = packages;
    }

    if (!yyjson_is_arr(arr))
    {
        yyjson_doc_free(doc);
        return NULL;
    }

    struct repo_index *idx = calloc(1, sizeof(*idx));
    if (!idx)
    {
        yyjson_doc_free(doc);
        return NULL;
    }

    idx->count = yyjson_arr_size(arr);
    idx->items = calloc(idx->count, sizeof(*idx->items));
    if (!idx->items)
    {
        free(idx);
        yyjson_doc_free(doc);
        return NULL;
    }

    size_t i = 0;
    yyjson_val *item;
    yyjson_arr_iter iter;
    yyjson_arr_iter_init(arr, &iter);
    while ((item = yyjson_arr_iter_next(&iter)) != NULL)
    {
        struct repo_package *pkg = &idx->items[i++];
        pkg->name = dup_str_or_empty(item, "name");
        pkg->version = dup_str_or_empty(item, "version");
        pkg->architecture = dup_str_or_empty(item, "architecture");
        pkg->channel = dup_str_or_empty(item, "channel");
        pkg->type = dup_str_or_empty(item, "type");
        pkg->description = dup_str_or_empty(item, "description");
        pkg->provides = dup_str_array(item, "provides", &pkg->provides_count);
        pkg->replaces = dup_str_array(item, "replaces", &pkg->replaces_count);
    }

    yyjson_doc_free(doc);
    return idx;
}

void
repo_index_free(struct repo_index *idx)
{
    if (!idx)
        return;

    for (size_t i = 0; i < idx->count; i++)
    {
        free(idx->items[i].name);
        free(idx->items[i].version);
        free(idx->items[i].architecture);
        free(idx->items[i].channel);
        free(idx->items[i].type);
        free(idx->items[i].description);
        for (size_t j = 0; j < idx->items[i].provides_count; j++)
            free(idx->items[i].provides[j]);
        free(idx->items[i].provides);
        for (size_t j = 0; j < idx->items[i].replaces_count; j++)
            free(idx->items[i].replaces[j]);
        free(idx->items[i].replaces);
    }
    free(idx->items);
    free(idx);
}

char *
repodata_cache_file(const char *cache_dir, const char *repo_url)
{
    unsigned int hash = fnv1a_hash(repo_url);

    char name[32];
    snprintf(name, sizeof(name), "%08x.json", hash);

    char *repodata_dir = path_join(cache_dir, "repodata");
    if (!repodata_dir)
        return NULL;

    char *full = path_join(repodata_dir, name);
    free(repodata_dir);
    return full;
}

bool
repodata_is_fresh(const char *cache_file, int ttl_seconds)
{
    struct stat st;
    if (stat(cache_file, &st) != 0)
        return false;

    time_t now = time(NULL);
    return (now - st.st_mtime) < ttl_seconds;
}

bool
repodata_refresh(const char *repo_url, const char *cache_dir)
{
    char url[2048];
    snprintf(url, sizeof(url), "%s/repodata.json", repo_url);

    struct http_response resp = {0};
    if (!http_get(url, &resp))
        return false;

    if (resp.status < 200 || resp.status >= 300)
    {
        http_response_free(&resp);
        return false;
    }

    char *cache_file = repodata_cache_file(cache_dir, repo_url);
    if (!cache_file)
    {
        http_response_free(&resp);
        return false;
    }

    char *repodata_dir = path_join(cache_dir, "repodata");
    if (repodata_dir)
    {
        mkdir_p(repodata_dir);
        free(repodata_dir);
    }

    FILE *f = fopen(cache_file, "wb");
    bool ok = f != NULL;
    if (f)
    {
        ok = fwrite(resp.body, 1, resp.len, f) == resp.len;
        fclose(f);
    }

    free(cache_file);
    http_response_free(&resp);
    return ok;
}

struct repo_index *
repodata_read_cache_file(const char *cache_file)
{
    FILE *f = fopen(cache_file, "rb");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0)
    {
        fclose(f);
        return NULL;
    }

    char *buf = malloc((size_t)size + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }

    size_t n = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[n] = '\0';

    struct repo_index *idx = repo_index_parse_json(buf, n);
    free(buf);
    return idx;
}

struct repo_index *
repodata_load(const char *repo_url, const char *cache_dir, int ttl_seconds,
              bool force_refresh)
{
    char *cache_file = repodata_cache_file(cache_dir, repo_url);
    if (!cache_file)
        return NULL;

    if (force_refresh || !repodata_is_fresh(cache_file, ttl_seconds))
        repodata_refresh(repo_url, cache_dir);

    struct repo_index *idx = repodata_read_cache_file(cache_file);
    free(cache_file);
    return idx;
}
