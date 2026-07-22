// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>

#include "http.h"
#include "../repo/repodata.h"

struct repo_index *api_search(const char *base_url, const char *query,
                              const char *arch, const char *channel,
                              const char *type);

struct repo_index *api_get_package(const char *base_url, const char *name);

struct repo_index *api_get_repodata(const char *base_url);

bool api_download(const char *base_url, const char *channel, const char *name,
                  const char *version, const char *arch, const char *dest_path,
                  http_progress_fn progress, void *userdata);

bool api_download_sig(const char *base_url, const char *channel,
                      const char *name, const char *version, const char *arch,
                      const char *dest_path);

bool api_health(const char *base_url);

char *api_version(const char *base_url);
