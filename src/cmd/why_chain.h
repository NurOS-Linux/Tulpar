// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <apg/db.h>

char **why_find_chain(struct db_handle *db, const char *name, int *count);

void why_free_names(char **names, int count);
