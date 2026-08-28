// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <yyjson.h>

#include "cmd_info.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../net/api.h"
#include "../repo/repo.h"
#include "../util/paths.h"
#include "../i18n.h"

#define USAGE "tulpar info [--dest <path>] [--json] <name>"

static void
print_str_list(const char *label, const struct str_list *list)
{
    if (!list || list->count == 0)
        return;

    printf("%-13s", label);
    for (int i = 0; i < list->count; i++)
        printf("%s%s", i > 0 ? ", " : "", list->items[i]);
    printf("\n");
}

static void
print_dependencies(const struct dep_constraint_list *deps)
{
    if (!deps || deps->count == 0)
        return;

    printf("Depends On:  ");
    for (int i = 0; i < deps->count; i++)
    {
        struct dep_constraint *c = &deps->items[i];
        if (i > 0)
            printf(", ");
        if (c->op == VER_OP_ANY || !c->version)
            printf("%s", c->name);
        else
        {
            const char *op_str = "=";
            switch (c->op)
            {
            case VER_OP_EQ:
                op_str = "=";
                break;
            case VER_OP_NEQ:
                op_str = "!=";
                break;
            case VER_OP_LT:
                op_str = "<";
                break;
            case VER_OP_LE:
                op_str = "<=";
                break;
            case VER_OP_GT:
                op_str = ">";
                break;
            case VER_OP_GE:
                op_str = ">=";
                break;
            default:
                break;
            }
            printf("%s %s %s", c->name, op_str, c->version);
        }
    }
    printf("\n");
}

static void
print_pkg_human(const struct package_metadata *m, const struct str_list *req_by)
{
    printf("Name:        %s\n", m->name ? m->name : "");
    printf("Version:     %s\n", m->version ? m->version : "");
    printf("Type:        %s\n", m->type ? m->type : "");
    printf("Arch:        %s\n", m->architecture ? m->architecture : "");
    printf("Maintainer:  %s\n", m->maintainer ? m->maintainer : "");
    printf("License:     %s\n", m->license ? m->license : "");
    printf("Homepage:    %s\n", m->homepage ? m->homepage : "");

    print_dependencies(&m->dependencies);
    print_str_list("Provides:    ", &m->provides);
    print_str_list("Conflicts:   ", &m->conflicts);
    print_str_list("Replaces:    ", &m->replaces);
    print_str_list("Required By: ", req_by);

    printf("Description: %s\n", m->description ? m->description : "");
}

static void
print_pkg_json(const struct package_metadata *m, const struct str_list *req_by)
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
            struct dep_constraint *c = &m->dependencies.items[i];
            if (c->op == VER_OP_ANY || !c->version)
                yyjson_mut_arr_add_strcpy(doc, dep_arr, c->name);
            else
            {
                char dep_str[256];
                const char *op_str = "=";
                switch (c->op)
                {
                case VER_OP_EQ:
                    op_str = "=";
                    break;
                case VER_OP_NEQ:
                    op_str = "!=";
                    break;
                case VER_OP_LT:
                    op_str = "<";
                    break;
                case VER_OP_LE:
                    op_str = "<=";
                    break;
                case VER_OP_GT:
                    op_str = ">";
                    break;
                case VER_OP_GE:
                    op_str = ">=";
                    break;
                default:
                    break;
                }
                snprintf(dep_str, sizeof(dep_str), "%s %s %s", c->name, op_str,
                         c->version);
                yyjson_mut_arr_add_strcpy(doc, dep_arr, dep_str);
            }
        }
        yyjson_mut_obj_add_val(doc, obj, "dependencies", dep_arr);
    }

    if (m->provides.count > 0)
    {
        yyjson_mut_val *prov_arr = yyjson_mut_arr(doc);
        for (int i = 0; i < m->provides.count; i++)
            yyjson_mut_arr_add_strcpy(doc, prov_arr, m->provides.items[i]);
        yyjson_mut_obj_add_val(doc, obj, "provides", prov_arr);
    }

    if (m->conflicts.count > 0)
    {
        yyjson_mut_val *conf_arr = yyjson_mut_arr(doc);
        for (int i = 0; i < m->conflicts.count; i++)
            yyjson_mut_arr_add_strcpy(doc, conf_arr, m->conflicts.items[i]);
        yyjson_mut_obj_add_val(doc, obj, "conflicts", conf_arr);
    }

    if (m->replaces.count > 0)
    {
        yyjson_mut_val *repl_arr = yyjson_mut_arr(doc);
        for (int i = 0; i < m->replaces.count; i++)
            yyjson_mut_arr_add_strcpy(doc, repl_arr, m->replaces.items[i]);
        yyjson_mut_obj_add_val(doc, obj, "replaces", repl_arr);
    }

    if (req_by && req_by->count > 0)
    {
        yyjson_mut_val *req_arr = yyjson_mut_arr(doc);
        for (int i = 0; i < req_by->count; i++)
            yyjson_mut_arr_add_strcpy(doc, req_arr, req_by->items[i]);
        yyjson_mut_obj_add_val(doc, obj, "required_by", req_arr);
    }

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

static struct str_list
find_required_by(struct db_handle *db, const char *target_name,
                 const struct str_list *provides)
{
    struct str_list req_by = {0};
    int count = 0;
    struct package **pkgs = db_list(db, &count);
    if (!pkgs)
        return req_by;

    for (int i = 0; i < count; i++)
    {
        struct package *p = pkgs[i];
        if (strcmp(p->meta->name, target_name) == 0)
            continue;

        bool matched = false;
        for (int d = 0; d < p->meta->dependencies.count && !matched; d++)
        {
            const char *dep_name = p->meta->dependencies.items[d].name;
            if (strcmp(dep_name, target_name) == 0)
                matched = true;
            else if (provides)
            {
                for (int pv = 0; pv < provides->count; pv++)
                {
                    if (strcmp(dep_name, provides->items[pv]) == 0)
                    {
                        matched = true;
                        break;
                    }
                }
            }
        }

        if (matched)
        {
            char **new_items =
                realloc(req_by.items, sizeof(char *) * (req_by.count + 1));
            if (new_items)
            {
                req_by.items = new_items;
                req_by.items[req_by.count++] = strdup(p->meta->name);
            }
        }
    }

    for (int i = 0; i < count; i++)
        package_free(pkgs[i]);
    free(pkgs);

    return req_by;
}

int
cmd_info_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    bool json_output = false;
    const char *name = NULL;
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
        else if (end_of_options || argv[i][0] != '-')
        {
            if (!name)
                name = argv[i];
            else
            {
                ui_errorf(_("unexpected argument: %s"), argv[i]);
                cmd_print_usage(USAGE);
                return 1;
            }
        }
        else
        {
            ui_errorf(_("unknown option: %s"), argv[i]);
            cmd_print_usage(USAGE);
            return 1;
        }
    }

    if (!name)
    {
        ui_error(_("info requires a package name"));
        cmd_print_usage(USAGE);
        return 1;
    }

    struct dest_ctx dest = {0};
    dest_ctx_resolve(dest_arg, cfg->db_dir, &dest);

    struct db_handle *db = db_open_readonly(dest.db_path);
    struct package *local = db ? db_get(db, name) : NULL;

    if (local)
    {
        struct str_list req_by =
            find_required_by(db, local->meta->name, &local->meta->provides);
        if (json_output)
            print_pkg_json(local->meta, &req_by);
        else
            print_pkg_human(local->meta, &req_by);

        for (int i = 0; i < req_by.count; i++)
            free(req_by.items[i]);
        free(req_by.items);

        package_free(local);
        if (db)
            db_close(db);
        dest_ctx_clear(&dest);
        return 0;
    }

    if (db)
        db_close(db);
    dest_ctx_clear(&dest);

    struct repo_list *repos = repo_list_load();
    struct repo_index *idx = NULL;
    for (int i = 0; repos && i < repos->count && !idx; i++)
        idx = api_get_package(repos->urls[i], name);
    repo_list_free(repos);

    if (!idx || idx->count == 0)
    {
        ui_errorf(_("package %s not found in local database or remote "
                    "repositories"),
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
        print_pkg_json(&meta, NULL);
    else
        print_pkg_human(&meta, NULL);

    repo_index_free(idx);
    return 0;
}
