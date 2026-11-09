// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd_info.h"
#include "cmd_common.h"
#include "json_out.h"
#include "resolve.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../net/api.h"
#include "../net/http.h"
#include "../repo/repo.h"
#include "../util/paths.h"
#include "../i18n.h"

#define USAGE "tulpar info [--dest <path>] [--json] <name>"

static char *
join_list(const struct str_list *list)
{
    if (!list || list->count == 0)
        return NULL;
    size_t len = 1;
    for (int i = 0; i < list->count; i++)
        len += strlen(list->items[i]) + 2;
    char *out = malloc(len);
    if (!out)
        return NULL;
    out[0] = '\0';
    for (int i = 0; i < list->count; i++)
    {
        if (i > 0)
            strcat(out, ", ");
        strcat(out, list->items[i]);
    }
    return out;
}

static char *
join_dependencies(const struct dep_constraint_list *deps, struct db_handle *db)
{
    if (!deps || deps->count == 0)
        return NULL;
    struct str_list parts = {0};
    parts.items = calloc((size_t)deps->count, sizeof(char *));
    if (!parts.items)
        return NULL;
    for (int i = 0; i < deps->count; i++)
    {
        char *dep = dep_constraint_to_str(&deps->items[i]);
        if (!dep)
            continue;
        if (db && !resolve_dep_installed(db, &deps->items[i]))
        {
            char buf[512];
            snprintf(buf, sizeof(buf), "%s (%s)", dep, _("not installed"));
            free(dep);
            dep = strdup(buf);
        }
        parts.items[parts.count++] = dep;
    }
    char *out = join_list(&parts);
    str_list_free(&parts);
    return out;
}

static void
print_pkg_human(const struct package_metadata *m, const char *status,
                const struct str_list *req_by, struct db_handle *db,
                const char *versions)
{
    ui_print_heading("%s %s", m->name ? m->name : "",
                     m->version ? m->version : "");
    if (m->description && m->description[0] != '\0')
        printf("%s\n", m->description);
    printf("\n");

    char *deps = join_dependencies(&m->dependencies, db);
    char *provides = join_list(&m->provides);
    char *conflicts = join_list(&m->conflicts);
    char *replaces = join_list(&m->replaces);
    char *required_by = join_list(req_by);

    const char *labels[] = {
        _("Status"),    _("Type"),     _("Architecture"), _("Maintainer"),
        _("License"),   _("Homepage"), _("Depends on"),   _("Provides"),
        _("Conflicts"), _("Replaces"), _("Required by"),  _("Versions"),
    };
    const char *values[] = {
        status,      m->type,  m->architecture, m->maintainer, m->license,
        m->homepage, deps,     provides,        conflicts,     replaces,
        required_by, versions,
    };
    size_t count = sizeof(labels) / sizeof(labels[0]);

    size_t width = 0;
    for (size_t i = 0; i < count; i++)
        if (values[i] && values[i][0] != '\0' &&
            ui_text_width(labels[i]) > width)
            width = ui_text_width(labels[i]);

    for (size_t i = 0; i < count; i++)
        ui_print_field(labels[i], width, values[i]);

    free(deps);
    free(provides);
    free(conflicts);
    free(replaces);
    free(required_by);
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
            cmd_unknown_option(argv[i], USAGE);
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
        char status[256];
        snprintf(status, sizeof(status), "%s%s%s",
                 local->installed_by_hand ? _("installed explicitly")
                                          : _("installed as a dependency"),
                 local->held ? ", " : "", local->held ? _("held") : "");
        if (json_output)
            json_out_emit(json_out_package_info(local->meta, &req_by));
        else
            print_pkg_human(local->meta, status, &req_by, db, NULL);

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
    {
        idx = api_get_package(repos->urls[i], name);
        if (!idx && !http_last_not_found())
            cmd_warn_repo_unreachable(repos->urls[i], false);
    }
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

    char versions[512] = "";
    for (size_t i = 0; i < idx->count; i++)
    {
        size_t len = strlen(versions);
        snprintf(versions + len, sizeof(versions) - len, "%s%s",
                 i > 0 ? ", " : "", idx->items[i].version);
    }

    if (json_output)
        json_out_emit(json_out_package_info(&meta, NULL));
    else
        print_pkg_human(&meta, _("not installed"), NULL, NULL,
                        idx->count > 1 ? versions : NULL);

    repo_index_free(idx);
    return 0;
}
