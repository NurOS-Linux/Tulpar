// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cmd_list.h"
#include "cmd_common.h"
#include "resolve.h"
#include "json_out.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../repo/repo.h"
#include "../util/paths.h"
#include "../i18n.h"

#define USAGE                                                                  \
    "tulpar list [--dest <path>] [--json] [--names] [--explicit | --deps] "    \
    "[--held] [--upgradable] [pattern]"

int
cmd_list_run(int argc, char **argv, struct tulpar_config *cfg)
{
    const char *dest_arg = NULL;
    const char *pattern = NULL;
    bool json_output = false;
    bool names_only = false;
    bool only_explicit = false;
    bool only_deps = false;
    bool only_held = false;
    bool only_upgradable = false;
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
        else if (!end_of_options && arg_is(argv[i], "names", '\0'))
            names_only = true;
        else if (!end_of_options && arg_is(argv[i], "explicit", '\0'))
            only_explicit = true;
        else if (!end_of_options && arg_is(argv[i], "deps", '\0'))
            only_deps = true;
        else if (!end_of_options && arg_is(argv[i], "held", '\0'))
            only_held = true;
        else if (!end_of_options && arg_is(argv[i], "upgradable", '\0'))
            only_upgradable = true;
        else if (end_of_options || argv[i][0] != '-')
        {
            if (!pattern)
                pattern = argv[i];
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

    if (names_only && json_output)
    {
        ui_error(_("--names and --json cannot be combined"));
        cmd_print_usage(USAGE);
        return 1;
    }

    if (only_explicit && only_deps)
    {
        ui_error(_("--explicit and --deps cannot be combined"));
        cmd_print_usage(USAGE);
        return 1;
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

    struct package **shown = calloc((size_t)count + 1, sizeof(*shown));
    char **available = calloc((size_t)count + 1, sizeof(*available));
    size_t shown_count = 0;
    struct repo_list *repos = only_upgradable ? repo_list_load() : NULL;
    bool ok = shown && available;

    for (int i = 0; ok && i < count; i++)
    {
        struct package *pkg = pkgs[i];
        if (pattern && strstr(pkg->meta->name, pattern) == NULL)
            continue;
        if (only_explicit && !pkg->installed_by_hand)
            continue;
        if (only_deps && pkg->installed_by_hand)
            continue;
        if (only_held && !pkg->held)
            continue;
        if (only_upgradable)
        {
            struct remote_pick pick;
            if (!resolve_lookup_named(pkg->meta->name, VER_OP_GT,
                                      pkg->meta->version, repos, cfg, &pick))
                continue;
            available[shown_count] = strdup(pick.version);
        }
        shown[shown_count++] = pkg;
    }

    if (!ok)
        ui_error(_("out of memory"));
    else if (names_only)
    {
        for (size_t i = 0; i < shown_count; i++)
            printf("%s\n", shown[i]->meta->name);
    }
    else if (json_output)
        json_out_emit(json_out_package_list(
            shown, shown_count,
            only_upgradable ? (const char *const *)available : NULL));
    else if (shown_count == 0)
    {
        if (pattern || only_explicit || only_deps || only_held ||
            only_upgradable)
            ui_info(_("no installed packages matched"));
        else
            ui_info(_("no packages installed"));
    }
    else
    {
        size_t name_width = 0;
        size_t version_width = 0;
        size_t tag_width = 0;
        for (size_t i = 0; i < shown_count; i++)
        {
            size_t w = ui_text_width(shown[i]->meta->name);
            if (w > name_width)
                name_width = w;
            w = ui_text_width(shown[i]->meta->version);
            if (w > version_width)
                version_width = w;
            if (shown[i]->held && ui_text_width(_("held")) > tag_width)
                tag_width = ui_text_width(_("held"));
            if (available[i] && ui_text_width(available[i]) + 2 > tag_width)
                tag_width = ui_text_width(available[i]) + 2;
        }

        for (size_t i = 0; i < shown_count; i++)
        {
            char tag[160] = "";
            if (available[i])
                snprintf(tag, sizeof(tag), "-> %s", available[i]);
            else if (shown[i]->held)
                snprintf(tag, sizeof(tag), "%s", _("held"));
            ui_print_package_row(shown[i]->meta->name, name_width,
                                 shown[i]->meta->version, version_width,
                                 tag[0] ? tag : NULL, tag_width,
                                 shown[i]->meta->description);
        }

        if (isatty(STDOUT_FILENO) != 0)
        {
            printf("\n");
            ui_infof(ngettext("%zu package", "%zu packages", shown_count),
                     shown_count);
        }
    }

    for (int i = 0; i < count; i++)
    {
        package_free(pkgs[i]);
        free(available ? available[i] : NULL);
    }
    free(pkgs);
    free(shown);
    free(available);
    repo_list_free(repos);

    db_close(db);
    dest_ctx_clear(&dest);
    return ok ? 0 : 1;
}
