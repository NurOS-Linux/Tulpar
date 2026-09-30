// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdint.h>
#include <string.h>

#include "suggest.h"

static size_t
edit_distance(const char *a, const char *b)
{
    size_t alen = strlen(a);
    size_t blen = strlen(b);
    if (alen > 32 || blen > 32)
        return SIZE_MAX;

    size_t row[33];
    for (size_t j = 0; j <= blen; j++)
        row[j] = j;

    for (size_t i = 1; i <= alen; i++)
    {
        size_t diag = row[0];
        row[0] = i;
        for (size_t j = 1; j <= blen; j++)
        {
            size_t above = row[j];
            size_t cost = a[i - 1] == b[j - 1] ? 0 : 1;
            size_t best = diag + cost;
            if (above + 1 < best)
                best = above + 1;
            if (row[j - 1] + 1 < best)
                best = row[j - 1] + 1;
            row[j] = best;
            diag = above;
        }
    }
    return row[blen];
}

const char *
suggest_closest(const char *token, const char *const *candidates, size_t count)
{
    const char *best = NULL;
    size_t best_distance = SIZE_MAX;

    for (size_t i = 0; i < count; i++)
    {
        size_t d = edit_distance(token, candidates[i]);
        if (d < best_distance)
        {
            best_distance = d;
            best = candidates[i];
        }
    }

    size_t limit = strlen(token) <= 4 ? 1 : 2;
    return best_distance > 0 && best_distance <= limit ? best : NULL;
}
