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

#define USAGE                                                                  \
    "tulpar search [--dest <path>] [--json] [--installed] [--remote] <query>"

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
    bool only_installed = false;
    bool only_remote = false;
    const char *query = NULL;
    bool end_of_options = false;

    for (int i = 0; i < argc; i++)
    {
        const char *value = NULL;
        if (!end_of_options && strcmp(argv[i], "--") == 0)
        {
            end_of_options = true;
            continue;
        }
        if (!end_of_options && arg_is_help(argv[i]))
        {
            cmd_print_usage(USAGE);
            return 0;
        }
        else if (!end_of_options &&
                 arg_take_value(argc, argv, &i, "dest", 'd', &value))
            dest_arg = value;
        else if (!end_of_options && arg_is(argv[i], "json", 'j'))
            json_output = true;
        else if (!end_of_options && strcmp(argv[i], "--installed") == 0)
            only_installed = true;
        else if (!end_of_options && strcmp(argv[i], "--remote") == 0)
            only_remote = true;
        else if (end_of_options || argv[i][0] != '-')
        {
            if (!query)
                query = argv[i];
            else
            {
                ui_errorf(_("unexpected argument: %s"), argv[i]);
                cmd_print_usage(USAGE);
                return 1;
            }
        }
        else
        {
            cmd_unknown_option(argv[i], USAGE);
            return 1;
        }
    }

    if (!query)
    {
        ui_error(_("search requires a query string"));
        cmd_print_usage(USAGE);
        return 1;
    }

    bool show_installed = !only_remote;
    bool show_remote = !only_installed;

    struct dest_ctx dest = {0};
    dest_ctx_resolve(dest_arg, cfg->db_dir, &dest);

    struct db_handle *db = db_open_readonly(dest.db_path);
    int local_count = 0;
    struct package **local =
        db && show_installed ? db_search(db, query, &local_count) : NULL;

    struct repo_list *repos = show_remote ? repo_list_load() : NULL;
    int repo_count = repos ? repos->count : 0;
    struct repo_index **indexes =
        repo_count > 0 ? calloc((size_t)repo_count, sizeof(*indexes)) : NULL;
    const struct repo_package **matches = NULL;
    size_t match_count = 0;
    size_t match_cap = 0;

    for (int r = 0; indexes && r < repo_count; r++)
    {
        bool refresh_failed = false;
        indexes[r] = repodata_load(repos->urls[r], cfg->cache_dir,
                                   cfg->repodata_ttl, false, &refresh_failed);
        if (refresh_failed)
            cmd_warn_repo_unreachable(repos->urls[r], indexes[r] != NULL);
        if (!indexes[r])
            continue;

        for (size_t i = 0; i < indexes[r]->count; i++)
        {
            const struct repo_package *pkg = &indexes[r]->items[i];
            if (!substr_ci(pkg->name, query) &&
                !substr_ci(pkg->description, query))
                continue;
            if (match_count == match_cap)
            {
                size_t cap = match_cap ? match_cap * 2 : 32;
                const struct repo_package **tmp =
                    realloc(matches, cap * sizeof(*matches));
                if (!tmp)
                    break;
                matches = tmp;
                match_cap = cap;
            }
            matches[match_count++] = pkg;
        }
    }

    if (json_output)
    {
        yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
        yyjson_mut_val *arr = yyjson_mut_arr(doc);
        yyjson_mut_doc_set_root(doc, arr);

        for (int i = 0; i < local_count; i++)
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
        for (size_t i = 0; i < match_count; i++)
        {
            const struct repo_package *pkg = matches[i];
            yyjson_mut_val *obj = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_str(doc, obj, "source", "remote");
            yyjson_mut_obj_add_strcpy(doc, obj, "name", pkg->name);
            yyjson_mut_obj_add_strcpy(doc, obj, "version", pkg->version);
            yyjson_mut_obj_add_strcpy(doc, obj, "description",
                                      pkg->description);
            yyjson_mut_arr_add_val(arr, obj);
        }

        char *json = yyjson_mut_write(doc, 0, NULL);
        if (json)
        {
            printf("%s\n", json);
            free(json);
        }
        yyjson_mut_doc_free(doc);
    }
    else if (local_count == 0 && match_count == 0)
    {
        ui_infof(_("no packages match '%s'"), query);
    }
    else
    {
        size_t name_width = 0;
        size_t version_width = 0;
        for (int i = 0; i < local_count; i++)
        {
            size_t w = ui_text_width(local[i]->meta->name);
            if (w > name_width)
                name_width = w;
            w = ui_text_width(local[i]->meta->version);
            if (w > version_width)
                version_width = w;
        }
        for (size_t i = 0; i < match_count; i++)
        {
            size_t w = ui_text_width(matches[i]->name);
            if (w > name_width)
                name_width = w;
            w = ui_text_width(matches[i]->version);
            if (w > version_width)
                version_width = w;
        }

        if (local_count > 0)
        {
            ui_print_heading(_("Installed (%d)"), local_count);
            for (int i = 0; i < local_count; i++)
                ui_print_package_row(local[i]->meta->name, name_width,
                                     local[i]->meta->version, version_width,
                                     NULL, 0, local[i]->meta->description);
        }

        char (*tags)[128] =
            match_count > 0 ? calloc(match_count, sizeof(*tags)) : NULL;
        size_t tag_width = 0;
        for (size_t i = 0; tags && i < match_count; i++)
        {
            struct package *installed =
                db ? db_get(db, matches[i]->name) : NULL;
            if (installed &&
                strcmp(installed->meta->version, matches[i]->version) == 0)
                snprintf(tags[i], sizeof(tags[i]), "%s", _("installed"));
            else if (installed)
                snprintf(tags[i], sizeof(tags[i]), _("installed %s"),
                         installed->meta->version);
            package_free(installed);
            size_t w = ui_text_width(tags[i]);
            if (w > tag_width)
                tag_width = w;
        }

        if (match_count > 0)
        {
            if (local_count > 0)
                printf("\n");
            ui_print_heading(_("Available (%zu)"), match_count);
            for (size_t i = 0; i < match_count; i++)
                ui_print_package_row(matches[i]->name, name_width,
                                     matches[i]->version, version_width,
                                     tags ? tags[i] : NULL, tag_width,
                                     matches[i]->description);
        }
        free(tags);
    }

    free(matches);
    for (int r = 0; indexes && r < repo_count; r++)
        repo_index_free(indexes[r]);
    free(indexes);

    for (int i = 0; i < local_count; i++)
        package_free(local[i]);
    free(local);
    if (db)
        db_close(db);
    if (repos)
        repo_list_free(repos);
    dest_ctx_clear(&dest);

    return 0;
}
