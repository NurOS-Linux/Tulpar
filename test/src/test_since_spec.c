// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <time.h>

#include "helpers.h"
#include "tests.h"
#include "util/since_spec.h"

void
test_since_spec_relative_units(void)
{
    time_t now = 1000000000;
    time_t out = 0;

    assert(since_spec_parse("30s", now, &out) && out == now - 30);
    assert(since_spec_parse("5m", now, &out) && out == now - 300);
    assert(since_spec_parse("2h", now, &out) && out == now - 7200);
    assert(since_spec_parse("7d", now, &out) && out == now - 7 * 86400);
    assert(since_spec_parse("2w", now, &out) && out == now - 14 * 86400);
    assert(since_spec_parse("0d", now, &out) && out == now);
    assert(since_spec_parse("99999999999d", now, &out) && out == 0);
    printf("test_since_spec_relative_units: PASS\n");
}

void
test_since_spec_dates_and_epoch(void)
{
    time_t out = 0;
    struct tm tm = {.tm_year = 126,
                    .tm_mon = 2,
                    .tm_mday = 14,
                    .tm_hour = 0,
                    .tm_min = 0,
                    .tm_isdst = -1};
    time_t midnight = mktime(&tm);
    tm.tm_hour = 9;
    tm.tm_min = 30;
    tm.tm_isdst = -1;
    time_t morning = mktime(&tm);

    assert(since_spec_parse("2026-03-14", 0, &out) && out == midnight);
    assert(since_spec_parse("2026-03-14 09:30", 0, &out) && out == morning);
    assert(since_spec_parse("2026-03-14T09:30", 0, &out) && out == morning);
    assert(since_spec_parse("@1700000000", 0, &out) && out == 1700000000);
    printf("test_since_spec_dates_and_epoch: PASS\n");
}

void
test_since_spec_rejects_malformed_input(void)
{
    time_t out = 0;
    const char *bad[] = {"",
                         "d",
                         "-5d",
                         "5",
                         "5x",
                         "5dd",
                         "2026-13-01",
                         "2026-03-32",
                         "2026-03-14 25:00",
                         "2026-03-14 9",
                         "2026-03-14x",
                         "@",
                         "@-1",
                         "@12x",
                         "abc"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++)
        assert(!since_spec_parse(bad[i], 1000000000, &out));
    assert(!since_spec_parse(NULL, 1000000000, &out));
    printf("test_since_spec_rejects_malformed_input: PASS\n");
}
