// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>

#include <yyjson.h>

#include "cmd_info.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../net/api.h"
#include "../repo/repo.h"
#include "../util/paths.h"

#define USAGE "tulpar info [--dest <path>] [--json] <name>"

static void
print_pkg_human(const struct package_metadata *m)
{
    printf("Name:        %s\n", m->name ? m->name : "");
    printf("Version:     %s\n", m->version ? m->version : "");
    printf("Type:        %s\n", m->type ? m->type : "");
    printf("Arch:        %s\n", m->architecture ? m->architecture : "");
    printf("Maintainer:  %s\n", m->maintainer ? m->maintainer : "");
    printf("License:     %s\n", m->license ? m->license : "");
    printf("Homepage:    %s\n", m->homepage ? m->homepage : "");
    printf("Description: %s\n", m->description ? m->description : "");
}

static void
print_pkg_json(const struct package_metadata *m)
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
    yyjson_mut_obj_add_strcpy(doc, obj, "description",
                              m->description ? m->description : "");

    char *json = yyjson_mut_write(doc, 0, NULL);
    if (json)
    {
        printf("%s\n", json);
        free(json);
    }
    yyjson_mut_doc_free(doc);
}

int
cmd_info_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    bool json_output = false;
    const char *name = NULL;

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
        else if (!name)
            name = argv[i];
    }

    if (!name)
    {
        ui_error("info requires a package name");
        cmd_print_usage(USAGE);
        return 1;
    }

    struct dest_ctx dest = {0};
    dest_ctx_resolve(dest_arg, cfg->db_dir, &dest);

    struct db_handle *db = db_open_readonly(dest.db_path);
    struct package *local = db ? db_get(db, name) : NULL;
    if (db)
        db_close(db);
    dest_ctx_clear(&dest);

    if (local)
    {
        if (json_output)
            print_pkg_json(local->meta);
        else
            print_pkg_human(local->meta);
        package_free(local);
        return 0;
    }

    struct repo_list *repos = repo_list_load();
    struct repo_index *idx = NULL;
    for (int i = 0; i < repos->count && !idx; i++)
        idx = api_get_package(repos->urls[i], name);
    repo_list_free(repos);

    if (!idx || idx->count == 0)
    {
        ui_errorf("package %s not found locally or in any configured repo",
                  name);
        if (idx)
            repo_index_free(idx);
        return 1;
    }

    struct repo_package *pkg = &idx->items[0];
    struct package_metadata meta = {
        .name = pkg->name,
        .version = pkg->version,
        .type = pkg->type,
        .architecture = pkg->architecture,
        .description = pkg->description,
    };

    if (json_output)
        print_pkg_json(&meta);
    else
        print_pkg_human(&meta);

    repo_index_free(idx);
    return 0;
}
