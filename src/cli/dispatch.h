// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stddef.h>

#include "../config/config.h"

struct command
{
    const char *name;
    char alias;
    const char *summary;
    int (*run)(int argc, char **argv, struct tulpar_config *cfg);
};

extern const struct command g_commands[];
extern const size_t g_command_count;

const struct command *command_lookup(const char *token);
