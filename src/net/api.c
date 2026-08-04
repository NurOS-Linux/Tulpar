// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <yyjson.h>

#include "api.h"

static char *
url_encode(const char *raw)
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return strdup(raw);

    char *escaped = curl_easy_escape(curl, raw, 0);
    char *out = escaped ? strdup(escaped) : strdup(raw);
    if (escaped)
        curl_free(escaped);
    curl_easy_cleanup(curl);
    return out;
}

struct repo_index *
api_search(const char *base_url, const char *query, const char *arch,
           const char *channel, const char *type)
{
    char url[2048];
    int off = snprintf(url, sizeof(url), "%s/api/v2/packages?", base_url);

    if (query && query[0])
    {
        char *enc = url_encode(query);
        off += snprintf(url + off, sizeof(url) - (size_t)off, "q=%s&", enc);
        free(enc);
    }
    if (arch && arch[0])
        off += snprintf(url + off, sizeof(url) - (size_t)off, "arch=%s&", arch);
    if (channel && channel[0])
        off += snprintf(url + off, sizeof(url) - (size_t)off, "channel=%s&",
                        channel);
    if (type && type[0])
        snprintf(url + off, sizeof(url) - (size_t)off, "type=%s", type);

    struct http_response resp = {0};
    if (!http_get(url, &resp))
        return NULL;

    struct repo_index *idx = NULL;
    if (resp.status >= 200 && resp.status < 300)
        idx = repo_index_parse_json(resp.body, resp.len);

    http_response_free(&resp);
    return idx;
}

static char *
dup_str_or_empty(yyjson_val *obj, const char *key)
{
    yyjson_val *v = yyjson_obj_get(obj, key);
    const char *s = yyjson_is_str(v) ? yyjson_get_str(v) : "";
    return strdup(s);
}

static struct repo_index *
parse_package_detail(const char *json, size_t len)
{
    yyjson_doc *doc = yyjson_read(json, len, 0);
    if (!doc)
        return NULL;

    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *name_val = yyjson_obj_get(root, "name");
    yyjson_val *builds = yyjson_obj_get(root, "builds");
    if (!yyjson_is_str(name_val) || !yyjson_is_arr(builds))
    {
        yyjson_doc_free(doc);
        return NULL;
    }

    const char *name = yyjson_get_str(name_val);

    struct repo_index *idx = calloc(1, sizeof(*idx));
    if (!idx)
    {
        yyjson_doc_free(doc);
        return NULL;
    }

    idx->count = yyjson_arr_size(builds);
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
    yyjson_arr_iter_init(builds, &iter);
    while ((item = yyjson_arr_iter_next(&iter)) != NULL)
    {
        struct repo_package *pkg = &idx->items[i++];
        pkg->name = strdup(name);
        pkg->version = dup_str_or_empty(item, "version");
        pkg->architecture = dup_str_or_empty(item, "architecture");
        pkg->channel = dup_str_or_empty(item, "channel");
        pkg->type = dup_str_or_empty(item, "type");
        pkg->description = dup_str_or_empty(item, "description");
    }

    yyjson_doc_free(doc);
    return idx;
}

struct repo_index *
api_get_package(const char *base_url, const char *name)
{
    char url[2048];
    char *enc = url_encode(name);
    snprintf(url, sizeof(url), "%s/api/v2/packages/%s", base_url, enc);
    free(enc);

    struct http_response resp = {0};
    if (!http_get(url, &resp))
        return NULL;

    struct repo_index *idx = NULL;
    if (resp.status >= 200 && resp.status < 300)
        idx = parse_package_detail(resp.body, resp.len);

    http_response_free(&resp);
    return idx;
}

struct repo_index *
api_get_repodata(const char *base_url)
{
    char url[2048];
    snprintf(url, sizeof(url), "%s/api/v2/repodata", base_url);

    struct http_response resp = {0};
    if (!http_get(url, &resp))
        return NULL;

    struct repo_index *idx = NULL;
    if (resp.status >= 200 && resp.status < 300)
        idx = repo_index_parse_json(resp.body, resp.len);

    http_response_free(&resp);
    return idx;
}

static void
build_download_url(char *out, size_t outlen, const char *base_url,
                   const char *channel, const char *name, const char *version,
                   const char *arch)
{
    snprintf(out, outlen, "%s/api/v2/download/%s/%s/%s/%s", base_url, channel,
             name, version, arch);
}

bool
api_download(const char *base_url, const char *channel, const char *name,
             const char *version, const char *arch, const char *dest_path,
             http_progress_fn progress, void *userdata)
{
    char url[2048];
    build_download_url(url, sizeof(url), base_url, channel, name, version,
                       arch);

    struct http_response resp = {0};
    return http_download(url, dest_path, progress, userdata, &resp);
}

bool
api_download_sig(const char *base_url, const char *channel, const char *name,
                 const char *version, const char *arch, const char *dest_path)
{
    char url[2048];
    build_download_url(url, sizeof(url), base_url, channel, name, version,
                       arch);
    strncat(url, ".sig", sizeof(url) - strlen(url) - 1);

    struct http_response resp = {0};
    return http_download(url, dest_path, NULL, NULL, &resp);
}

bool
api_health(const char *base_url)
{
    char url[2048];
    snprintf(url, sizeof(url), "%s/api/v2/health", base_url);

    struct http_response resp = {0};
    bool ok = http_get(url, &resp) && resp.status >= 200 && resp.status < 300;

    http_response_free(&resp);
    return ok;
}

char *
api_version(const char *base_url)
{
    char url[2048];
    snprintf(url, sizeof(url), "%s/api/v2/version", base_url);

    struct http_response resp = {0};
    if (!http_get(url, &resp))
        return NULL;

    char *version = NULL;
    if (resp.status >= 200 && resp.status < 300)
    {
        yyjson_doc *doc = yyjson_read(resp.body, resp.len, 0);
        if (doc)
        {
            yyjson_val *root = yyjson_doc_get_root(doc);
            yyjson_val *v = yyjson_obj_get(root, "version");
            if (yyjson_is_str(v))
                version = strdup(yyjson_get_str(v));
            yyjson_doc_free(doc);
        }
    }

    http_response_free(&resp);
    return version;
}
