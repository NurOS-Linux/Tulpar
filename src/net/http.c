// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "http.h"

static _Thread_local char g_error[CURL_ERROR_SIZE + 256];
static _Thread_local long g_status;
static bool g_offline;

void
http_set_offline(bool offline)
{
    g_offline = offline;
}

bool
http_is_offline(void)
{
    return g_offline;
}

void
http_set_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_error, sizeof(g_error), fmt, ap);
    va_end(ap);
}

const char *
http_last_error(void)
{
    return g_error[0] ? g_error : "unknown network error";
}

bool
http_last_not_found(void)
{
    return g_status == 404;
}

static void
clear_error(void)
{
    g_error[0] = '\0';
    g_status = 0;
}

static void
set_transfer_error(CURLcode res, const char *errbuf)
{
    http_set_error("%s", errbuf[0] ? errbuf : curl_easy_strerror(res));
}

static void
set_status_error(long status)
{
    g_status = status;
    http_set_error("server returned HTTP %ld", status);
}

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
    if (g_offline)
    {
        clear_error();
        http_set_error("offline mode, network access is disabled");
        return false;
    }
    clear_error();
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        http_set_error("cannot initialise libcurl");
        return false;
    }

    char errbuf[CURL_ERROR_SIZE] = "";
    struct write_buf buf = {0};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
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
        if (status < 200 || status >= 300)
            set_status_error(status);
    }
    else
    {
        set_transfer_error(res, errbuf);
        free(buf.data);
    }

    curl_easy_cleanup(curl);
    return ok;
}

bool
http_download(const char *url, const char *dest_path, http_progress_fn progress,
              void *userdata, struct http_response *out)
{
    if (g_offline)
    {
        clear_error();
        http_set_error("offline mode, network access is disabled");
        return false;
    }
    clear_error();
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        http_set_error("cannot initialise libcurl");
        return false;
    }

    FILE *f = fopen(dest_path, "wb");
    if (!f)
    {
        http_set_error("cannot create %s: %s", dest_path, strerror(errno));
        curl_easy_cleanup(curl);
        return false;
    }

    char errbuf[CURL_ERROR_SIZE] = "";

    struct progress_ctx pctx = {.cb = progress, .userdata = userdata};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
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
    int write_errno = errno;
    bool closed = fclose(f) == 0;

    bool ok = res == CURLE_OK;
    if (res == CURLE_WRITE_ERROR)
        http_set_error("cannot write %s: %s", dest_path, strerror(write_errno));
    else if (!ok)
        set_transfer_error(res, errbuf);
    else if (!closed)
    {
        http_set_error("cannot write %s: %s", dest_path, strerror(errno));
        ok = false;
    }
    if (ok && out)
    {
        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        out->status = status;
        out->body = NULL;
        out->len = 0;
        ok = status >= 200 && status < 300;
        if (!ok)
            set_status_error(status);
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
