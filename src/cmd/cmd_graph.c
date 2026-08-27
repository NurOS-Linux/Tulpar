// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apg/graph.h>

#include "cmd_graph.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../i18n.h"

#define USAGE "tulpar graph [--dest <path>] [-o <file>] [package]"

int
cmd_graph_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    const char *out_path = NULL;
    const char *target_pkg = NULL;

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
        else if (arg_take_value(argc, argv, &i, "output", 'o', &value))
            out_path = value;
        else if (!target_pkg)
            target_pkg = argv[i];
    }

    struct dest_ctx dest = {0};
    dest_ctx_resolve(dest_arg, cfg->db_dir, &dest);

    struct db_handle *db = cmd_open_db(&dest, false);
    if (!db)
    {
        dest_ctx_clear(&dest);
        return 1;
    }

    int count = 0;
    struct package **pkgs = db_list(db, &count);

    struct dep_graph *g = dep_graph_new();
    if (!g)
    {
        ui_error(_("failed to allocate dependency graph"));
        for (int i = 0; i < count; i++)
            package_free(pkgs[i]);
        free(pkgs);
        db_close(db);
        dest_ctx_clear(&dest);
        return 1;
    }

    for (int i = 0; i < count; i++)
        dep_graph_add_installed(g, pkgs[i]->meta);

    char *dot = NULL;

    if (target_pkg)
    {
        struct package *target = db_get(db, target_pkg);
        if (!target)
        {
            ui_errorf(_("package %s is not installed"), target_pkg);
            dep_graph_free(g);
            for (int i = 0; i < count; i++)
                package_free(pkgs[i]);
            free(pkgs);
            db_close(db);
            dest_ctx_clear(&dest);
            return 1;
        }
        package_free(target);

        char **order = NULL;
        size_t order_count = 0;
        dep_error_t dep_err =
            dep_graph_resolve(g, target_pkg, &order, &order_count);
        if (dep_err != DEP_OK)
        {
            ui_errorf(_("failed to resolve dependency graph for %s"),
                      target_pkg);
            dep_graph_free(g);
            for (int i = 0; i < count; i++)
                package_free(pkgs[i]);
            free(pkgs);
            db_close(db);
            dest_ctx_clear(&dest);
            return 1;
        }

        struct dep_graph *sub_g = dep_graph_new();
        if (!sub_g)
        {
            ui_error(_("failed to allocate dependency graph"));
            free(order);
            dep_graph_free(g);
            for (int i = 0; i < count; i++)
                package_free(pkgs[i]);
            free(pkgs);
            db_close(db);
            dest_ctx_clear(&dest);
            return 1;
        }

        for (size_t i = 0; i < order_count; i++)
        {
            for (int j = 0; j < count; j++)
            {
                if (strcmp(pkgs[j]->meta->name, order[i]) == 0)
                {
                    dep_graph_add_installed(sub_g, pkgs[j]->meta);
                    break;
                }
            }
        }

        free(order);
        dep_graph_free(g);

        dot = dep_graph_export_dot(sub_g);
        dep_graph_free(sub_g);
    }
    else
    {
        dot = dep_graph_export_dot(g);
        dep_graph_free(g);
    }

    for (int i = 0; i < count; i++)
        package_free(pkgs[i]);
    free(pkgs);
    db_close(db);
    dest_ctx_clear(&dest);

    if (!dot)
    {
        ui_error(_("failed to export the dependency graph"));
        return 1;
    }

    if (out_path)
    {
        FILE *f = fopen(out_path, "wb");
        if (!f)
        {
            ui_errorf(_("failed to open %s for writing"), out_path);
            free(dot);
            return 1;
        }
        fputs(dot, f);
        fclose(f);
        ui_successf(_("wrote dependency graph to %s"), out_path);
    }
    else
    {
        fputs(dot, stdout);
    }

    free(dot);
    return 0;
}
