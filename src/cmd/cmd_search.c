// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <yyjson.h>

#include "cmd_search.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../repo/repo.h"
#include "../repo/repodata.h"
#include "../util/paths.h"
#include "../i18n.h"

#define USAGE "tulpar search [--dest <path>] [--json] <query>"

static bool
substr_ci(const char *haystack, const char *needle)
{
    if (!haystack || !needle || needle[0] == '\0')
        return false;

    size_t hlen = strlen(haystack);
    size_t nlen = strlen(needle);
    if (nlen > hlen)
        return false;

    for (size_t i = 0; i + nlen <= hlen; i++)
    {
        size_t j = 0;
        while (j < nlen && tolower((unsigned char)haystack[i + j]) ==
                               tolower((unsigned char)needle[j]))
            j++;
        if (j == nlen)
            return true;
    }
    return false;
}

int
cmd_search_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    bool json_output = false;
    const char *query = NULL;

    for (int i = 0; i < argc; i++)
    {
        const char *value = NULL;
        if (arg_is_help(argv[i]))
        {
            cmd_print_usage(USAGE);
            return 0;
        }
        else if (arg_take_value(argc, argv, &i, "dest", 'd', &value))
            dest_arg = value;
        else if (arg_is(argv[i], "json", 'j'))
            json_output = true;
        else if (!query)
            query = argv[i];
    }

    if (!query)
    {
        ui_error(_("search requires a query string"));
        cmd_print_usage(USAGE);
        return 1;
    }

    struct dest_ctx dest = {0};
    dest_ctx_resolve(dest_arg, cfg->db_dir, &dest);

    struct db_handle *db = db_open_readonly(dest.db_path);
    int local_count = 0;
    struct package **local = db ? db_search(db, query, &local_count) : NULL;

    struct repo_list *repos = repo_list_load();

    yyjson_mut_doc *doc = NULL;
    yyjson_mut_val *arr = NULL;
    if (json_output)
    {
        doc = yyjson_mut_doc_new(NULL);
        arr = yyjson_mut_arr(doc);
        yyjson_mut_doc_set_root(doc, arr);
    }
    else
    {
        printf("Installed:\n");
    }

    for (int i = 0; i < local_count; i++)
    {
        struct package *pkg = local[i];
        if (json_output)
        {
            yyjson_mut_val *obj = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_str(doc, obj, "source", "local");
            yyjson_mut_obj_add_strcpy(doc, obj, "name", pkg->meta->name);
            yyjson_mut_obj_add_strcpy(doc, obj, "version", pkg->meta->version);
            yyjson_mut_obj_add_strcpy(
                doc, obj, "description",
                pkg->meta->description ? pkg->meta->description : "");
            yyjson_mut_arr_add_val(arr, obj);
        }
        else
        {
            printf("  %s %s\n    %s\n", pkg->meta->name, pkg->meta->version,
                   pkg->meta->description ? pkg->meta->description : "");
        }
    }

    if (!json_output)
        printf("\nAvailable:\n");

    for (int r = 0; r < repos->count; r++)
    {
        struct repo_index *idx = repodata_load(repos->urls[r], cfg->cache_dir,
                                               cfg->repodata_ttl, false);
        if (!idx)
            continue;

        for (size_t i = 0; i < idx->count; i++)
        {
            struct repo_package *pkg = &idx->items[i];
            if (!substr_ci(pkg->name, query) &&
                !substr_ci(pkg->description, query))
                continue;

            if (json_output)
            {
                yyjson_mut_val *obj = yyjson_mut_obj(doc);
                yyjson_mut_obj_add_str(doc, obj, "source", "remote");
                yyjson_mut_obj_add_strcpy(doc, obj, "name", pkg->name);
                yyjson_mut_obj_add_strcpy(doc, obj, "version", pkg->version);
                yyjson_mut_obj_add_strcpy(doc, obj, "description",
                                          pkg->description);
                yyjson_mut_arr_add_val(arr, obj);
            }
            else
            {
                printf("  %s %s\n    %s\n", pkg->name, pkg->version,
                       pkg->description);
            }
        }

        repo_index_free(idx);
    }

    if (json_output)
    {
        char *json = yyjson_mut_write(doc, 0, NULL);
        if (json)
        {
            printf("%s\n", json);
            free(json);
        }
        yyjson_mut_doc_free(doc);
    }

    for (int i = 0; i < local_count; i++)
        package_free(local[i]);
    free(local);
    if (db)
        db_close(db);
    repo_list_free(repos);
    dest_ctx_clear(&dest);

    return 0;
}
