// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>
#include <stddef.h>

struct http_response
{
    long status;
    char *body;
    size_t len;
};

typedef void (*http_progress_fn)(size_t now, size_t total, void *userdata);

bool http_global_init(void);
void http_global_cleanup(void);

bool http_get(const char *url, struct http_response *out);

bool http_download(const char *url, const char *dest_path,
                   http_progress_fn progress, void *userdata,
                   struct http_response *out);

void http_response_free(struct http_response *resp);
