// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apg/db.h>
#include <apg/package.h>
#include <apg/version.h>
#include <yyjson.h>

#include "cmd/json_out.h"
#include "cmd/why_chain.h"
#include "helpers.h"
#include "tests.h"

static void
add_pkg(struct db_handle *db, const char *name, const char *dep, bool by_hand)
{
    struct package *pkg = package_new();
    assert(pkg);
    pkg->meta->name = strdup(name);
    pkg->meta->version = strdup("1.0.0");
    pkg->installed_by_hand = by_hand;
    if (dep)
    {
        pkg->meta->dependencies.items =
            calloc(1, sizeof(struct dep_constraint));
        pkg->meta->dependencies.items[0] = dep_constraint_parse(dep);
        pkg->meta->dependencies.count = 1;
    }
    assert(db_add(db, pkg));
    package_free(pkg);
}

void
test_why_chain_follows_dependents_to_explicit_package(void)
{
    char *dir = mktmp_dir("why");
    struct db_handle *db = db_open(dir);
    assert(db);

    add_pkg(db, "top", "mid", true);
    add_pkg(db, "mid", "leaf", false);
    add_pkg(db, "leaf", NULL, false);
    add_pkg(db, "stray", NULL, false);

    int count = 0;
    char **chain = why_find_chain(db, "leaf", &count);
    assert(chain);
    assert(count == 3);
    assert(strcmp(chain[0], "top") == 0);
    assert(strcmp(chain[1], "mid") == 0);
    assert(strcmp(chain[2], "leaf") == 0);

    char *required[] = {"mid"};
    char *json = json_out_why("leaf", false, required, 1, chain, count);
    yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
    assert(doc);
    yyjson_val *arr = yyjson_obj_get(yyjson_doc_get_root(doc), "chain");
    assert(yyjson_arr_size(arr) == 3);
    assert(strcmp(yyjson_get_str(yyjson_arr_get(arr, 0)), "top") == 0);
    assert(strcmp(yyjson_get_str(yyjson_arr_get(arr, 2)), "leaf") == 0);
    assert(
        !yyjson_get_bool(yyjson_obj_get(yyjson_doc_get_root(doc), "orphan")));
    yyjson_doc_free(doc);
    free(json);
    why_free_names(chain, count);

    chain = why_find_chain(db, "stray", &count);
    assert(chain == NULL);
    assert(count == 0);

    json = json_out_why("stray", false, NULL, 0, NULL, 0);
    doc = yyjson_read(json, strlen(json), 0);
    assert(doc);
    assert(yyjson_get_bool(yyjson_obj_get(yyjson_doc_get_root(doc), "orphan")));
    yyjson_doc_free(doc);
    free(json);

    db_close(db);
    char cmd[300];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", dir);
    assert(system(cmd) == 0);
    free(dir);
    printf("test_why_chain_follows_dependents_to_explicit_package: PASS\n");
}
