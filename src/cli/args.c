// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <string.h>

#include "args.h"

bool
arg_is(const char *arg, const char *long_name, char short_name)
{
    if (arg[0] != '-')
        return false;

    if (arg[1] == '-')
        return long_name && strcmp(arg + 2, long_name) == 0;

    return short_name != '\0' && arg[1] == short_name && arg[2] == '\0';
}

bool
arg_is_help(const char *arg)
{
    return arg_is(arg, "help", 'h');
}

bool
arg_take_value(int argc, char **argv, int *i, const char *long_name,
               char short_name, const char **out_value)
{
    const char *arg = argv[*i];
    if (arg[0] != '-')
        return false;

    if (arg[1] == '-')
    {
        size_t llen = long_name ? strlen(long_name) : 0;
        if (!long_name || strncmp(arg + 2, long_name, llen) != 0)
            return false;

        if (arg[2 + llen] == '=')
        {
            *out_value = arg + 2 + llen + 1;
            return true;
        }
        if (arg[2 + llen] != '\0')
            return false;

        if (*i + 1 >= argc)
            return false;
        *out_value = argv[++(*i)];
        return true;
    }

    if (short_name == '\0' || arg[1] != short_name)
        return false;

    if (arg[2] != '\0')
    {
        *out_value = arg + 2;
        return true;
    }

    if (*i + 1 >= argc)
        return false;
    *out_value = argv[++(*i)];
    return true;
}
