// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <unistd.h>

#include <apg/db.h>
#include <apg/package.h>
#include <apg/transaction.h>

#include "cli/ui.h"
#include "helpers.h"
#include "tests.h"

static struct db_handle *
open_tmp_db(char **out_path)
{
    *out_path = mktmp_dir("plan-db");
    return db_open(*out_path);
}

static void
close_tmp_db(struct db_handle *db, const char *path)
{
    char f[512];
    db_close(db);
    snprintf(f, sizeof(f), "%s/data.mdb", path);
    unlink(f);
    snprintf(f, sizeof(f), "%s/lock.mdb", path);
    unlink(f);
    rmdir(path);
}

void
test_plan_render_install_step(void)
{
    char *db_path = NULL;
    struct db_handle *db = open_tmp_db(&db_path);
    assert(db);

    struct package *pkg = package_new();
    assert(pkg);
    pkg->meta->name = strdup("demo-plan-pkg");
    pkg->meta->version = strdup("3.2.1");

    struct apg_trans *trans = trans_new(db);
    assert(trans);
    assert(trans_add_install(trans, pkg) == TRANS_OK);
    assert(trans_prepare(trans) == TRANS_OK);

    ui_init(NULL);

    char *out_path = mktmp_path("plan-out");
    fflush(stdout);
    int saved_stdout = dup(STDOUT_FILENO);
    assert(saved_stdout >= 0);
    FILE *captured = freopen(out_path, "w", stdout);
    assert(captured);

    ui_print_plan(trans);

    fflush(stdout);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    clearerr(stdout);

    FILE *check = fopen(out_path, "r");
    assert(check);
    char buf[4096] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, check);
    (void)n;
    fclose(check);

    assert(strstr(buf, "demo-plan-pkg") != NULL);
    assert(strstr(buf, "3.2.1") != NULL);
    assert(strstr(buf, "install") != NULL);

    unlink(out_path);
    free(out_path);

    trans_free(trans);
    package_free(pkg);
    close_tmp_db(db, db_path);
    free(db_path);

    printf("test_plan_render_install_step: PASS\n");
}
