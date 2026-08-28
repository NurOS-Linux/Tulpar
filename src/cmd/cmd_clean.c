// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <apg/copy.h>

#include "cmd_clean.h"
#include "cmd_common.h"
#include "../cli/args.h"
#include "../cli/ui.h"
#include "../i18n.h"
#include "../util/paths.h"

#define USAGE "tulpar clean [-y]"

static size_t
clean_directory_contents(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir)
        return 0;

    size_t removed_count = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        char *child = path_join(path, ent->d_name);
        if (!child)
            continue;

        struct stat st;
        if (lstat(child, &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                remove_dir_recursive(child);
                removed_count++;
            }
            else
            {
                if (unlink(child) == 0)
                    removed_count++;
            }
        }
        free(child);
    }

    closedir(dir);
    return removed_count;
}

int
cmd_clean_run(int argc, char **argv, struct tulpar_config *cfg)
{
    bool assume_yes = false;

    for (int i = 0; i < argc; i++)
    {
        if (arg_is_help(argv[i]))
        {
            cmd_print_usage(USAGE);
            return 0;
        }
        else if (arg_is(argv[i], "yes", 'y'))
            assume_yes = true;
    }

    char prompt[512];
    snprintf(prompt, sizeof(prompt), "%s (%s)?",
             _("Purge downloaded packages and metadata cache"), cfg->cache_dir);

    if (!ui_confirm(prompt, assume_yes))
    {
        ui_info(_("aborted"));
        return 0;
    }

    size_t total_removed = 0;

    char *pkgs_dir = path_join(cfg->cache_dir, "pkgs");
    if (pkgs_dir)
    {
        total_removed += clean_directory_contents(pkgs_dir);
        free(pkgs_dir);
    }

    char *git_tmp_dir = path_join(cfg->cache_dir, "git-tmp");
    if (git_tmp_dir)
    {
        total_removed += clean_directory_contents(git_tmp_dir);
        free(git_tmp_dir);
    }

    total_removed += clean_directory_contents(cfg->cache_dir);

    ui_successf(_("cache cleaned: %zu item(s) removed"), total_removed);
    return 0;
}
