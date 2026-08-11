// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "http.h"

struct write_buf
{
    char *data;
    size_t len;
    size_t cap;
};

static size_t
write_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    struct write_buf *buf = userdata;
    size_t add = size * nmemb;
    size_t need = buf->len + add + 1;

    if (need > buf->cap)
    {
        size_t new_cap = buf->cap == 0 ? 4096 : buf->cap;
        while (new_cap < need)
            new_cap *= 2;
        char *tmp = realloc(buf->data, new_cap);
        if (!tmp)
            return 0;
        buf->data = tmp;
        buf->cap = new_cap;
    }

    memcpy(buf->data + buf->len, ptr, add);
    buf->len += add;
    buf->data[buf->len] = '\0';
    return add;
}

struct progress_ctx
{
    http_progress_fn cb;
    void *userdata;
};

static int
xfer_cb(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
        curl_off_t ulnow)
{
    struct progress_ctx *ctx = clientp;
    if (!ctx->cb)
        return 0;

    if (dltotal > 0 || dlnow > 0)
        ctx->cb((size_t)dlnow, (size_t)dltotal, ctx->userdata);
    else if (ultotal > 0 || ulnow > 0)
        ctx->cb((size_t)ulnow, (size_t)ultotal, ctx->userdata);

    return 0;
}

bool
http_global_init(void)
{
    return curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
}

void
http_global_cleanup(void)
{
    curl_global_cleanup();
}

bool
http_get(const char *url, struct http_response *out)
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return false;

    struct write_buf buf = {0};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "tulpar/" TULPAR_VERSION);

    CURLcode res = curl_easy_perform(curl);
    bool ok = res == CURLE_OK;

    if (ok)
    {
        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        out->status = status;
        out->body = buf.data;
        out->len = buf.len;
    }
    else
    {
        free(buf.data);
    }

    curl_easy_cleanup(curl);
    return ok;
}

bool
http_download(const char *url, const char *dest_path, http_progress_fn progress,
              void *userdata, struct http_response *out)
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return false;

    FILE *f = fopen(dest_path, "wb");
    if (!f)
    {
        curl_easy_cleanup(curl);
        return false;
    }

    struct progress_ctx pctx = {.cb = progress, .userdata = userdata};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "tulpar/" TULPAR_VERSION);
    if (progress)
    {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xfer_cb);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &pctx);
    }

    CURLcode res = curl_easy_perform(curl);
    fclose(f);

    bool ok = res == CURLE_OK;
    if (ok && out)
    {
        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        out->status = status;
        out->body = NULL;
        out->len = 0;
        ok = status >= 200 && status < 300;
    }

    curl_easy_cleanup(curl);
    if (!ok)
        remove(dest_path);
    return ok;
}

void
http_response_free(struct http_response *resp)
{
    if (!resp)
        return;
    free(resp->body);
    resp->body = NULL;
    resp->len = 0;
}
