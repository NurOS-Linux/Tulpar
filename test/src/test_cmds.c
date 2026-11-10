// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <apg/db.h>
#include <apg/package.h>
#include <apg/version.h>

#include "cli/ui.h"
#include "cmd/cmd_files.h"
#include "cmd/cmd_mark.h"
#include "cmd/cmd_owns.h"
#include "cmd/cmd_reinstall.h"
#include "config/config.h"
#include "helpers.h"
#include "tests.h"

static void
add_pkg(struct db_handle *db, const char *name, const char *version,
        const char *file, bool by_hand)
{
    struct package *pkg = package_new();
    assert(pkg);
    pkg->meta->name = strdup(name);
    pkg->meta->version = strdup(version);
    pkg->installed_by_hand = by_hand;
    pkg->package_files.items = calloc(1, sizeof(char *));
    pkg->package_files.items[0] = strdup(file);
    pkg->package_files.count = 1;
    assert(db_add(db, pkg));
    package_free(pkg);
}

static char *
make_root(void)
{
    char *root = mktmp_dir("cmds");
    char path[512];
    snprintf(path, sizeof(path), "%s/var", root);
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/var/lib", root);
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/var/lib/apg", root);
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/var/lib/apg/db", root);
    mkdir(path, 0755);

    struct db_handle *db = db_open(path);
    assert(db);
    add_pkg(db, "alpha", "1.0.0", "/usr/bin/alpha", true);
    add_pkg(db, "beta", "2.0.0", "/usr/bin/beta", false);
    db_close(db);
    return root;
}

static void
remove_root(char *root)
{
    char cmd[600];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", root);
    assert(system(cmd) == 0);
    free(root);
}

static bool
by_hand(const char *root, const char *name)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/var/lib/apg/db", root);
    struct db_handle *db = db_open_readonly(path);
    assert(db);
    struct package *pkg = db_get(db, name);
    assert(pkg);
    bool value = pkg->installed_by_hand;
    package_free(pkg);
    db_close(db);
    return value;
}

static int
run(int (*fn)(int, char **, struct tulpar_config *), int argc, char **argv)
{
    struct tulpar_config *cfg = tulpar_config_defaults();
    assert(cfg);
    int rc = fn(argc, argv, cfg);
    tulpar_config_free(cfg);
    return rc;
}

void
test_cmd_mark_switches_installed_by_hand(void)
{
    ui_init(NULL, UI_COLOR_NEVER);
    char *root = make_root();

    assert(by_hand(root, "alpha"));
    char *a1[] = {"--dest", root, "--auto", "alpha"};
    assert(run(cmd_mark_run, 4, a1) == 0);
    assert(!by_hand(root, "alpha"));

    char *a2[] = {"--dest", root, "--explicit", "alpha", "beta"};
    assert(run(cmd_mark_run, 5, a2) == 0);
    assert(by_hand(root, "alpha"));
    assert(by_hand(root, "beta"));

    char *a3[] = {"--dest", root, "alpha"};
    assert(run(cmd_mark_run, 3, a3) == 1);
    char *a4[] = {"--dest", root, "--auto", "--explicit", "alpha"};
    assert(run(cmd_mark_run, 5, a4) == 1);
    char *a5[] = {"--dest", root, "--auto", "ghost"};
    assert(run(cmd_mark_run, 4, a5) == 1);
    char *a6[] = {"--dest", root, "--auto"};
    assert(run(cmd_mark_run, 3, a6) == 1);

    remove_root(root);
    printf("test_cmd_mark_switches_installed_by_hand: PASS\n");
}

void
test_cmd_owns_and_files_report_the_database(void)
{
    ui_init(NULL, UI_COLOR_NEVER);
    char *root = make_root();

    char *o1[] = {"--dest", root, "/usr/bin/alpha"};
    assert(run(cmd_owns_run, 3, o1) == 0);

    char prefixed[600];
    snprintf(prefixed, sizeof(prefixed), "%s/usr/bin/beta", root);
    char *o2[] = {"--dest", root, prefixed};
    assert(run(cmd_owns_run, 3, o2) == 0);

    char *o3[] = {"--dest", root, "/usr/bin/alpha", "/usr/bin/none"};
    assert(run(cmd_owns_run, 4, o3) == 1);
    char *o4[] = {"--dest", root, "/usr/bin/none", "/usr/bin/alpha", "--json"};
    assert(run(cmd_owns_run, 5, o4) == 1);
    char *o5[] = {"--dest", root};
    assert(run(cmd_owns_run, 2, o5) == 1);

    char *f1[] = {"--dest", root, "alpha"};
    assert(run(cmd_files_run, 3, f1) == 0);
    char *f2[] = {"--dest", root, "ghost"};
    assert(run(cmd_files_run, 3, f2) == 1);
    char *f3[] = {"--dest", root, "alpha", "beta"};
    assert(run(cmd_files_run, 4, f3) == 1);

    remove_root(root);
    printf("test_cmd_owns_and_files_report_the_database: PASS\n");
}

static char *
build_archive(const char *dir, const char *name, const char *version)
{
    char path[600];
    snprintf(path, sizeof(path), "%s/src-%s-%s/data/usr/bin", dir, name,
             version);
    char cmd[1400];
    snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", path);
    assert(system(cmd) == 0);

    char file[700];
    snprintf(file, sizeof(file), "%s/%s", path, name);
    write_file_contents(file, "payload\n");

    char meta[600];
    snprintf(meta, sizeof(meta), "%s/src-%s-%s/metadata.json", dir, name,
             version);
    char json[300];
    snprintf(json, sizeof(json),
             "{\"name\": \"%s\", \"version\": \"%s\", \"type\": \"app\"}", name,
             version);
    write_file_contents(meta, json);

    char archive[600];
    snprintf(archive, sizeof(archive), "%s/%s-%s.apg", dir, name, version);
    snprintf(cmd, sizeof(cmd),
             "tar -czf '%s' -C '%s/src-%s-%s' metadata.json data", archive, dir,
             name, version);
    assert(system(cmd) == 0);
    return strdup(archive);
}

void
test_cmd_reinstall_restores_files_from_an_archive(void)
{
    ui_init(NULL, UI_COLOR_NEVER);
    char *root = make_root();
    char *same = build_archive(root, "alpha", "1.0.0");
    char *other = build_archive(root, "alpha", "9.9.9");
    char *unknown = build_archive(root, "ghost", "1.0.0");

    char installed[600];
    snprintf(installed, sizeof(installed), "%s/usr/bin/alpha", root);
    assert(access(installed, F_OK) != 0);

    char *r1[] = {"--dest", root, "-y", other};
    assert(run(cmd_reinstall_run, 4, r1) == 1);
    char *r2[] = {"--dest", root, "-y", unknown};
    assert(run(cmd_reinstall_run, 4, r2) == 1);
    char *r3[] = {"--dest", root, "-y", "ghost"};
    assert(run(cmd_reinstall_run, 4, r3) == 1);
    char *r4[] = {"--dest", root, "-y"};
    assert(run(cmd_reinstall_run, 3, r4) == 1);
    assert(access(installed, F_OK) != 0);

    char *r5[] = {"--dest", root, "-y", "--dry-run", same};
    assert(run(cmd_reinstall_run, 5, r5) == 0);
    assert(access(installed, F_OK) != 0);

    char *r6[] = {"--dest", root, "-y", same};
    assert(run(cmd_reinstall_run, 4, r6) == 0);
    assert(access(installed, F_OK) == 0);
    assert(by_hand(root, "alpha"));

    free(same);
    free(other);
    free(unknown);
    remove_root(root);
    printf("test_cmd_reinstall_restores_files_from_an_archive: PASS\n");
}
