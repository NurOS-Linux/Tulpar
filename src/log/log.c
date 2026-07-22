// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#if defined(__linux__)
#include <syslog.h>
#endif

#include "log.h"

static FILE *g_log_file = NULL;
static bool g_verbose = false;
static bool g_quiet = false;
static bool g_journald_mirror = false;

static const char *
level_str(log_level_t level)
{
    switch (level)
    {
    case TULPAR_LOG_DEBUG:
        return "DEBUG";
    case TULPAR_LOG_INFO:
        return "INFO";
    case TULPAR_LOG_WARN:
        return "WARN";
    case TULPAR_LOG_ERROR:
        return "ERROR";
    default:
        return "?";
    }
}

bool
log_open(const char *path)
{
    if (g_log_file)
        fclose(g_log_file);

    g_log_file = fopen(path, "a");
    return g_log_file != NULL;
}

void
log_close(void)
{
    if (g_log_file)
    {
        fclose(g_log_file);
        g_log_file = NULL;
    }

#if defined(__linux__)
    if (g_journald_mirror)
        closelog();
#endif
}

void
log_set_verbosity(bool verbose, bool quiet)
{
    g_verbose = verbose;
    g_quiet = quiet;
}

void
log_set_journald_mirror(bool enabled)
{
    g_journald_mirror = enabled;

#if defined(__linux__)
    if (enabled)
        openlog("tulpar", 0, LOG_USER);
#endif
}

#if defined(__linux__)
static int
syslog_priority(log_level_t level)
{
    switch (level)
    {
    case TULPAR_LOG_DEBUG:
        return LOG_DEBUG;
    case TULPAR_LOG_INFO:
        return LOG_INFO;
    case TULPAR_LOG_WARN:
        return LOG_WARNING;
    case TULPAR_LOG_ERROR:
        return LOG_ERR;
    default:
        return LOG_INFO;
    }
}
#endif

void
log_write(log_level_t level, const char *msg)
{
    if (level == TULPAR_LOG_DEBUG && !g_verbose)
        return;
    if (g_quiet && level < TULPAR_LOG_ERROR)
        return;

    if (g_log_file)
    {
        time_t now = time(NULL);
        char timebuf[32];
        struct tm tm_buf;
        localtime_r(&now, &tm_buf);
        strftime(timebuf, sizeof(timebuf), "%b %e %H:%M:%S", &tm_buf);

        fprintf(g_log_file, "%s tulpar[%d]: %s: %s\n", timebuf, (int)getpid(),
                level_str(level), msg);
        fflush(g_log_file);
    }

#if defined(__linux__)
    if (g_journald_mirror)
        syslog(syslog_priority(level), "%s", msg);
#endif
}
