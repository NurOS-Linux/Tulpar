// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static inline char *
mktmp_path(const char *suffix)
{
    char tmpl[256];
    snprintf(tmpl, sizeof(tmpl), "/tmp/tulpar-test-%s-XXXXXX", suffix);
    int fd = mkstemp(tmpl);
    assert(fd >= 0);
    close(fd);
    return strdup(tmpl);
}

static inline char *
mktmp_dir(const char *suffix)
{
    char tmpl[256];
    snprintf(tmpl, sizeof(tmpl), "/tmp/tulpar-test-%s-XXXXXX", suffix);
    char *made = mkdtemp(tmpl);
    assert(made);
    return strdup(made);
}

static inline void
write_file_contents(const char *path, const char *content)
{
    FILE *f = fopen(path, "wb");
    assert(f);
    fwrite(content, 1, strlen(content), f);
    fclose(f);
}
