// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <yyjson.h>

#include "json_out.h"

void
json_out_emit(char *json)
{
    if (!json)
        return;
    printf("%s\n", json);
    free(json);
}

static char *
finish(yyjson_mut_doc *doc)
{
    char *json = yyjson_mut_write(doc, 0, NULL);
    yyjson_mut_doc_free(doc);
    return json;
}

const char *
json_out_op_name(journal_op_t op)
{
    switch (op)
    {
    case JOURNAL_INSTALL:
        return "install";
    case JOURNAL_REMOVE:
        return "remove";
    case JOURNAL_ROLLBACK:
        return "rollback";
    default:
        return "?";
    }
}

const char *
json_out_status_name(journal_status_t status)
{
    return status == JOURNAL_STATUS_OK ? "ok" : "failed";
}

char *
json_out_package_list(struct package *const *pkgs, size_t count)
{
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *arr = yyjson_mut_arr(doc);
    yyjson_mut_doc_set_root(doc, arr);

    for (size_t i = 0; i < count; i++)
    {
        yyjson_mut_val *obj = yyjson_mut_arr_add_obj(doc, arr);
        yyjson_mut_obj_add_strcpy(doc, obj, "name", pkgs[i]->meta->name);
        yyjson_mut_obj_add_strcpy(doc, obj, "version", pkgs[i]->meta->version);
        yyjson_mut_obj_add_strcpy(
            doc, obj, "description",
            pkgs[i]->meta->description ? pkgs[i]->meta->description : "");
        yyjson_mut_obj_add_bool(doc, obj, "installed_by_hand",
                                pkgs[i]->installed_by_hand);
        yyjson_mut_obj_add_bool(doc, obj, "held", pkgs[i]->held);
    }

    return finish(doc);
}

static void
add_string_array(yyjson_mut_doc *doc, yyjson_mut_val *obj, const char *key,
                 char *const *items, int count)
{
    yyjson_mut_val *arr = yyjson_mut_arr(doc);
    for (int i = 0; i < count; i++)
        yyjson_mut_arr_add_strcpy(doc, arr, items[i]);
    yyjson_mut_obj_add_val(doc, obj, key, arr);
}

char *
json_out_package_info(const struct package_metadata *m,
                      const struct str_list *req_by)
{
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *obj = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, obj);

    yyjson_mut_obj_add_strcpy(doc, obj, "name", m->name ? m->name : "");
    yyjson_mut_obj_add_strcpy(doc, obj, "version",
                              m->version ? m->version : "");
    yyjson_mut_obj_add_strcpy(doc, obj, "type", m->type ? m->type : "");
    yyjson_mut_obj_add_strcpy(doc, obj, "architecture",
                              m->architecture ? m->architecture : "");
    yyjson_mut_obj_add_strcpy(doc, obj, "maintainer",
                              m->maintainer ? m->maintainer : "");
    yyjson_mut_obj_add_strcpy(doc, obj, "license",
                              m->license ? m->license : "");
    yyjson_mut_obj_add_strcpy(doc, obj, "homepage",
                              m->homepage ? m->homepage : "");

    if (m->dependencies.count > 0)
    {
        yyjson_mut_val *dep_arr = yyjson_mut_arr(doc);
        for (int i = 0; i < m->dependencies.count; i++)
        {
            char *dep = dep_constraint_to_str(&m->dependencies.items[i]);
            if (dep)
                yyjson_mut_arr_add_strcpy(doc, dep_arr, dep);
            free(dep);
        }
        yyjson_mut_obj_add_val(doc, obj, "dependencies", dep_arr);
    }

    if (m->provides.count > 0)
        add_string_array(doc, obj, "provides", m->provides.items,
                         m->provides.count);
    if (m->conflicts.count > 0)
        add_string_array(doc, obj, "conflicts", m->conflicts.items,
                         m->conflicts.count);
    if (m->replaces.count > 0)
        add_string_array(doc, obj, "replaces", m->replaces.items,
                         m->replaces.count);
    if (req_by && req_by->count > 0)
        add_string_array(doc, obj, "required_by", req_by->items, req_by->count);

    yyjson_mut_obj_add_strcpy(doc, obj, "description",
                              m->description ? m->description : "");

    return finish(doc);
}

char *
json_out_history(struct journal_entry *const *entries, size_t count)
{
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *arr = yyjson_mut_arr(doc);
    yyjson_mut_doc_set_root(doc, arr);

    for (size_t i = 0; i < count; i++)
    {
        struct journal_entry *e = entries[i];
        const char *name = journal_entry_pkg_name(e);
        const char *version = journal_entry_pkg_version(e);

        yyjson_mut_val *obj = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_str(doc, obj, "op",
                               json_out_op_name(journal_entry_op(e)));
        yyjson_mut_obj_add_strcpy(doc, obj, "name", name ? name : "");
        yyjson_mut_obj_add_strcpy(doc, obj, "version", version ? version : "");
        yyjson_mut_obj_add_str(doc, obj, "status",
                               json_out_status_name(journal_entry_status(e)));
        yyjson_mut_obj_add_uint(doc, obj, "timestamp",
                                (uint64_t)journal_entry_timestamp(e));
        yyjson_mut_obj_add_bool(doc, obj, "explicit",
                                journal_entry_explicit(e));
        yyjson_mut_arr_add_val(arr, obj);
    }

    return finish(doc);
}

char *
json_out_search(struct package *const *local, size_t local_count,
                const struct repo_package *const *remote, size_t remote_count)
{
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *arr = yyjson_mut_arr(doc);
    yyjson_mut_doc_set_root(doc, arr);

    for (size_t i = 0; i < local_count; i++)
    {
        struct package *pkg = local[i];
        yyjson_mut_val *obj = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_str(doc, obj, "source", "local");
        yyjson_mut_obj_add_strcpy(doc, obj, "name", pkg->meta->name);
        yyjson_mut_obj_add_strcpy(doc, obj, "version", pkg->meta->version);
        yyjson_mut_obj_add_strcpy(
            doc, obj, "description",
            pkg->meta->description ? pkg->meta->description : "");
        yyjson_mut_arr_add_val(arr, obj);
    }
    for (size_t i = 0; i < remote_count; i++)
    {
        const struct repo_package *pkg = remote[i];
        yyjson_mut_val *obj = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_str(doc, obj, "source", "remote");
        yyjson_mut_obj_add_strcpy(doc, obj, "name", pkg->name);
        yyjson_mut_obj_add_strcpy(doc, obj, "version", pkg->version);
        yyjson_mut_obj_add_strcpy(doc, obj, "description", pkg->description);
        yyjson_mut_arr_add_val(arr, obj);
    }

    return finish(doc);
}
