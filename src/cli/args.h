// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdbool.h>

bool arg_is(const char *arg, const char *long_name, char short_name);

bool arg_is_help(const char *arg);

bool arg_take_value(int argc, char **argv, int *i, const char *long_name,
                    char short_name, const char **out_value);
