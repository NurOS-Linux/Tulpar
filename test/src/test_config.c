// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <unistd.h>

#include "config/config.h"
#include "helpers.h"
#include "tests.h"

void
test_config_defaults(void)
{
    struct tulpar_config *cfg = tulpar_config_defaults();
    assert(cfg);
    assert(cfg->repodata_ttl > 0);
    assert(cfg->max_parallel_downloads > 0);
    assert(cfg->require_signature == false);
    assert(strcmp(cfg->color_theme, "cyan") == 0);
    tulpar_config_free(cfg);
    printf("test_config_defaults: PASS\n");
}

void
test_config_parse_file_overrides(void)
{
    char *path = mktmp_path("tulpar-conf");
    write_file_contents(path, "# override test\n"
                              "repodata_ttl = 120\n"
                              "max_parallel_downloads = 8\n"
                              "require_signature = true\n"
                              "color_theme = magenta\n"
                              "quiet = yes\n");

    struct tulpar_config *cfg = tulpar_config_defaults();
    assert(cfg);
    assert(tulpar_config_parse_file(path, cfg));

    assert(cfg->repodata_ttl == 120);
    assert(cfg->max_parallel_downloads == 8);
    assert(cfg->require_signature == true);
    assert(strcmp(cfg->color_theme, "magenta") == 0);
    assert(cfg->quiet == true);

    tulpar_config_free(cfg);
    unlink(path);
    free(path);
    printf("test_config_parse_file_overrides: PASS\n");
}
