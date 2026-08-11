// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>

#include <apg/graph.h>

#include "cmd_graph.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"

#define USAGE "tulpar graph [--dest <path>] [-o <file>]"

int
cmd_graph_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    const char *out_path = NULL;

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
        ui_error("failed to allocate dependency graph");
        for (int i = 0; i < count; i++)
            package_free(pkgs[i]);
        free(pkgs);
        db_close(db);
        dest_ctx_clear(&dest);
        return 1;
    }

    for (int i = 0; i < count; i++)
        dep_graph_add_installed(g, pkgs[i]->meta);

    char *dot = dep_graph_export_dot(g);
    dep_graph_free(g);

    for (int i = 0; i < count; i++)
        package_free(pkgs[i]);
    free(pkgs);
    db_close(db);
    dest_ctx_clear(&dest);

    if (!dot)
    {
        ui_error("failed to export the dependency graph");
        return 1;
    }

    if (out_path)
    {
        FILE *f = fopen(out_path, "wb");
        if (!f)
        {
            ui_errorf("failed to open %s for writing", out_path);
            free(dot);
            return 1;
        }
        fputs(dot, f);
        fclose(f);
        ui_successf("wrote dependency graph to %s", out_path);
    }
    else
    {
        fputs(dot, stdout);
    }

    free(dot);
    return 0;
}
