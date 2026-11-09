// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stddef.h>

#include <apg/journal.h>
#include <apg/package.h>

#include "../repo/repodata.h"

void json_out_emit(char *json);

const char *json_out_op_name(journal_op_t op);

const char *json_out_status_name(journal_status_t status);

char *json_out_package_list(struct package *const *pkgs, size_t count);

char *json_out_package_info(const struct package_metadata *meta,
                            const struct str_list *required_by);

char *json_out_history(struct journal_entry *const *entries, size_t count);

char *json_out_owners(const char *const *paths, char *const *owners,
                      size_t count);

char *json_out_search(struct package *const *local, size_t local_count,
                      const struct repo_package *const *remote,
                      size_t remote_count);
