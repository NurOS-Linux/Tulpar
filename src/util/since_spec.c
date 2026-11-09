// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "since_spec.h"

static bool
parse_relative(const char *spec, time_t now, time_t *out)
{
    char *end = NULL;
    errno = 0;
    long long amount = strtoll(spec, &end, 10);
    if (errno != 0 || end == spec || amount < 0 || !end[0] || end[1])
        return false;

    long long unit;
    switch (end[0])
    {
    case 's':
        unit = 1;
        break;
    case 'm':
        unit = 60;
        break;
    case 'h':
        unit = 3600;
        break;
    case 'd':
        unit = 86400;
        break;
    case 'w':
        unit = 604800;
        break;
    default:
        return false;
    }

    if (amount > (long long)now / unit)
        *out = 0;
    else
        *out = now - (time_t)(amount * unit);
    return true;
}

static bool
parse_date(const char *spec, time_t *out)
{
    struct tm tm = {0};
    int year, month, day;
    int hour = 0;
    int minute = 0;
    int used = 0;

    if (sscanf(spec, "%4d-%2d-%2d%n", &year, &month, &day, &used) != 3)
        return false;
    if (spec[used] == ' ' || spec[used] == 'T')
    {
        int used_time = 0;
        if (sscanf(spec + used + 1, "%2d:%2d%n", &hour, &minute, &used_time) !=
            2)
            return false;
        used += 1 + used_time;
    }
    if (spec[used] != '\0')
        return false;
    if (month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 ||
        minute > 59)
        return false;

    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_isdst = -1;

    time_t t = mktime(&tm);
    if (t == (time_t)-1)
        return false;
    *out = t;
    return true;
}

static bool
parse_epoch(const char *spec, time_t *out)
{
    char *end = NULL;
    errno = 0;
    long long value = strtoll(spec + 1, &end, 10);
    if (errno != 0 || end == spec + 1 || *end != '\0' || value < 0)
        return false;
    *out = (time_t)value;
    return true;
}

bool
since_spec_parse(const char *spec, time_t now, time_t *out)
{
    if (!spec || !spec[0] || !out)
        return false;
    if (spec[0] == '@')
        return parse_epoch(spec, out);
    if (strchr(spec, '-'))
        return parse_date(spec, out);
    return parse_relative(spec, now, out);
}
