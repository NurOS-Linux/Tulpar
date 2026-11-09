// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apg/journal.h>
#include <apg/package.h>
#include <yyjson.h>

#include "cmd/json_out.h"
#include "helpers.h"
#include "tests.h"

static yyjson_doc *
parse(char *json)
{
    assert(json);
    yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
    free(json);
    assert(doc);
    return doc;
}

static struct package *
make_pkg(const char *name, const char *version, const char *desc, bool by_hand,
         bool held)
{
    struct package *pkg = package_new();
    assert(pkg);
    pkg->meta->name = strdup(name);
    pkg->meta->version = strdup(version);
    pkg->meta->description = desc ? strdup(desc) : NULL;
    pkg->installed_by_hand = by_hand;
    pkg->held = held;
    return pkg;
}

void
test_json_out_op_and_status_names(void)
{
    assert(strcmp(json_out_op_name(JOURNAL_INSTALL), "install") == 0);
    assert(strcmp(json_out_op_name(JOURNAL_REMOVE), "remove") == 0);
    assert(strcmp(json_out_op_name(JOURNAL_ROLLBACK), "rollback") == 0);
    assert(strcmp(json_out_status_name(JOURNAL_STATUS_OK), "ok") == 0);
    assert(strcmp(json_out_status_name(JOURNAL_STATUS_FAILED), "failed") == 0);
    printf("test_json_out_op_and_status_names: PASS\n");
}

void
test_json_out_package_list_schema(void)
{
    struct package *a = make_pkg("alpha", "1.0.0", "first", true, false);
    struct package *b = make_pkg("beta", "2.1.0", NULL, false, true);
    struct package *pkgs[] = {a, b};

    yyjson_doc *doc = parse(json_out_package_list(pkgs, 2));
    yyjson_val *root = yyjson_doc_get_root(doc);
    assert(yyjson_is_arr(root));
    assert(yyjson_arr_size(root) == 2);

    yyjson_val *first = yyjson_arr_get(root, 0);
    assert(strcmp(yyjson_get_str(yyjson_obj_get(first, "name")), "alpha") == 0);
    assert(strcmp(yyjson_get_str(yyjson_obj_get(first, "version")), "1.0.0") ==
           0);
    assert(strcmp(yyjson_get_str(yyjson_obj_get(first, "description")),
                  "first") == 0);
    assert(yyjson_get_bool(yyjson_obj_get(first, "installed_by_hand")));
    assert(!yyjson_get_bool(yyjson_obj_get(first, "held")));

    yyjson_val *second = yyjson_arr_get(root, 1);
    assert(strcmp(yyjson_get_str(yyjson_obj_get(second, "description")), "") ==
           0);
    assert(!yyjson_get_bool(yyjson_obj_get(second, "installed_by_hand")));
    assert(yyjson_get_bool(yyjson_obj_get(second, "held")));

    yyjson_doc_free(doc);
    package_free(a);
    package_free(b);

    doc = parse(json_out_package_list(NULL, 0));
    assert(yyjson_is_arr(yyjson_doc_get_root(doc)));
    assert(yyjson_arr_size(yyjson_doc_get_root(doc)) == 0);
    yyjson_doc_free(doc);
    printf("test_json_out_package_list_schema: PASS\n");
}

void
test_json_out_package_info_schema(void)
{
    struct package *pkg = make_pkg("gamma", "3.0.0", "third", false, false);
    pkg->meta->type = strdup("app");
    pkg->meta->provides.items = calloc(1, sizeof(char *));
    pkg->meta->provides.items[0] = strdup("gamma-virtual");
    pkg->meta->provides.count = 1;

    char *req_items[] = {"delta"};
    struct str_list req_by = {.items = req_items, .count = 1};

    yyjson_doc *doc = parse(json_out_package_info(pkg->meta, &req_by));
    yyjson_val *root = yyjson_doc_get_root(doc);
    assert(yyjson_is_obj(root));
    assert(strcmp(yyjson_get_str(yyjson_obj_get(root, "name")), "gamma") == 0);
    assert(strcmp(yyjson_get_str(yyjson_obj_get(root, "type")), "app") == 0);
    assert(strcmp(yyjson_get_str(yyjson_obj_get(root, "license")), "") == 0);
    assert(yyjson_arr_size(yyjson_obj_get(root, "provides")) == 1);
    assert(yyjson_arr_size(yyjson_obj_get(root, "required_by")) == 1);
    assert(yyjson_obj_get(root, "dependencies") == NULL);
    assert(yyjson_obj_get(root, "conflicts") == NULL);
    yyjson_doc_free(doc);

    doc = parse(json_out_package_info(pkg->meta, NULL));
    assert(yyjson_obj_get(yyjson_doc_get_root(doc), "required_by") == NULL);
    yyjson_doc_free(doc);

    package_free(pkg);
    printf("test_json_out_package_info_schema: PASS\n");
}

void
test_json_out_history_and_search_schema(void)
{
    yyjson_doc *doc = parse(json_out_history(NULL, 0));
    assert(yyjson_is_arr(yyjson_doc_get_root(doc)));
    assert(yyjson_arr_size(yyjson_doc_get_root(doc)) == 0);
    yyjson_doc_free(doc);

    struct package *local =
        make_pkg("epsilon", "1.2.3", "local one", true, false);
    struct package *locals[] = {local};
    struct repo_package remote = {
        .name = "zeta", .version = "0.9", .description = "remote one"};
    const struct repo_package *remotes[] = {&remote};

    doc = parse(json_out_search(locals, 1, remotes, 1));
    yyjson_val *root = yyjson_doc_get_root(doc);
    assert(yyjson_arr_size(root) == 2);
    assert(strcmp(yyjson_get_str(
                      yyjson_obj_get(yyjson_arr_get(root, 0), "source")),
                  "local") == 0);
    assert(strcmp(yyjson_get_str(
                      yyjson_obj_get(yyjson_arr_get(root, 1), "source")),
                  "remote") == 0);
    assert(
        strcmp(yyjson_get_str(yyjson_obj_get(yyjson_arr_get(root, 1), "name")),
               "zeta") == 0);
    yyjson_doc_free(doc);
    package_free(local);
    printf("test_json_out_history_and_search_schema: PASS\n");
}
