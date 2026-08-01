// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ui.h"

#define COL_RESET "\x1b[0m"
#define COL_BOLD "\x1b[1m"
#define COL_RED "\x1b[31m"
#define COL_GREEN "\x1b[32m"
#define COL_YELLOW "\x1b[33m"
#define COL_BLUE "\x1b[34m"
#define COL_MAGENTA "\x1b[35m"
#define COL_CYAN "\x1b[36m"

static bool g_colors_enabled = false;
static bool g_verbose = false;
static bool g_quiet = false;
static const char *g_accent = COL_CYAN;

static size_t g_progress_total = 0;
static size_t g_progress_current = 0;
static char g_progress_label[128];
static bool g_progress_active = false;

static const char *
accent_from_theme(const char *theme)
{
    if (!theme)
        return COL_CYAN;
    if (strcmp(theme, "blue") == 0)
        return COL_BLUE;
    if (strcmp(theme, "magenta") == 0)
        return COL_MAGENTA;
    if (strcmp(theme, "green") == 0)
        return COL_GREEN;
    if (strcmp(theme, "yellow") == 0)
        return COL_YELLOW;
    if (strcmp(theme, "red") == 0)
        return COL_RED;
    return COL_CYAN;
}

void
ui_init(const char *color_theme)
{
    g_accent = accent_from_theme(color_theme);

    if (getenv("NO_COLOR") != NULL)
    {
        g_colors_enabled = false;
        return;
    }

    g_colors_enabled = isatty(STDOUT_FILENO) != 0;
}

bool
ui_colors_enabled(void)
{
    return g_colors_enabled;
}

void
ui_set_verbosity(bool verbose, bool quiet)
{
    g_verbose = verbose;
    g_quiet = quiet;
}

static void
emit(FILE *stream, const char *color, const char *label, const char *msg)
{
    if (g_colors_enabled)
        fprintf(stream, "%s%s%s %s\n", color, label, COL_RESET, msg);
    else
        fprintf(stream, "%s %s\n", label, msg);
}

void
ui_success(const char *msg)
{
    if (g_quiet)
        return;
    emit(stdout, COL_GREEN, "==>", msg);
}

void
ui_error(const char *msg)
{
    emit(stderr, COL_RED, "error:", msg);
}

void
ui_warn(const char *msg)
{
    if (g_quiet)
        return;
    emit(stderr, COL_YELLOW, "warning:", msg);
}

void
ui_info(const char *msg)
{
    if (g_quiet)
        return;
    emit(stdout, g_accent, "::", msg);
}

void
ui_debug(const char *msg)
{
    if (!g_verbose)
        return;
    emit(stderr, COL_BLUE, "debug:", msg);
}

void
ui_successf(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ui_success(buf);
}

void
ui_errorf(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ui_error(buf);
}

void
ui_warnf(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ui_warn(buf);
}

void
ui_infof(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ui_info(buf);
}

void
ui_debugf(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ui_debug(buf);
}

bool
ui_confirm(const char *prompt, bool assume_yes)
{
    if (assume_yes)
        return true;

    if (g_colors_enabled)
        printf("%s%s%s %s [y/N] ", COL_BOLD, "::", COL_RESET, prompt);
    else
        printf(":: %s [y/N] ", prompt);
    fflush(stdout);

    char line[16];
    if (!fgets(line, sizeof(line), stdin))
        return false;

    return line[0] == 'y' || line[0] == 'Y';
}

void
ui_progress_start(const char *label, size_t total)
{
    g_progress_total = total;
    g_progress_current = 0;
    g_progress_active = isatty(STDOUT_FILENO) != 0 && !g_quiet;
    snprintf(g_progress_label, sizeof(g_progress_label), "%s", label);

    if (!g_progress_active)
        return;

    printf("%s: 0%%\r", g_progress_label);
    fflush(stdout);
}

void
ui_progress_update(size_t current)
{
    g_progress_current = current;
    if (!g_progress_active)
        return;

    int pct =
        g_progress_total > 0 ? (int)((current * 100) / g_progress_total) : 0;

    const int bar_width = 30;
    int filled = (pct * bar_width) / 100;

    printf("%s: [", g_progress_label);
    for (int i = 0; i < bar_width; i++)
        putchar(i < filled ? '#' : '-');
    printf("] %3d%%\r", pct);
    fflush(stdout);
}

void
ui_progress_finish(void)
{
    if (g_progress_active)
        printf("\n");
    g_progress_active = false;
}

static const char *
op_label(trans_op_t op)
{
    switch (op)
    {
    case TRANS_OP_INSTALL:
        return "install";
    case TRANS_OP_UPGRADE:
        return "upgrade";
    case TRANS_OP_REMOVE:
        return "remove";
    default:
        return "?";
    }
}

static const char *
op_color(trans_op_t op)
{
    switch (op)
    {
    case TRANS_OP_INSTALL:
        return COL_GREEN;
    case TRANS_OP_UPGRADE:
        return COL_YELLOW;
    case TRANS_OP_REMOVE:
        return COL_RED;
    default:
        return COL_RESET;
    }
}

void
ui_print_plan(const struct apg_trans *trans)
{
    size_t count = trans_plan_count(trans);

    if (count == 0)
    {
        ui_info("nothing to do");
        return;
    }

    printf("\nTransaction plan:\n");
    for (size_t i = 0; i < count; i++)
    {
        const struct trans_step *s = trans_plan_at(trans, i);
        trans_op_t op = trans_step_op(s);
        const char *name = trans_step_pkg_name(s);
        const char *ver = trans_step_pkg_version(s);
        if (!ver)
            ver = "";

        if (g_colors_enabled)
            printf("  %s%-8s%s %s %s\n", op_color(op), op_label(op), COL_RESET,
                   name, ver);
        else
            printf("  %-8s %s %s\n", op_label(op), name, ver);
    }
    printf("\n");
}

void
ui_print_conflicts(const struct apg_trans *trans)
{
    size_t conflict_count = trans_conflict_count(trans);
    for (size_t i = 0; i < conflict_count; i++)
    {
        const struct trans_conflict *c = trans_conflict_at(trans, i);
        ui_errorf("%s conflicts with installed package %s",
                  trans_conflict_pkg_name(c), trans_conflict_conflicts_with(c));
    }

    size_t file_conflict_count = trans_file_conflict_count(trans);
    for (size_t i = 0; i < file_conflict_count; i++)
    {
        const struct trans_file_conflict *fc = trans_file_conflict_at(trans, i);
        ui_errorf("file %s requested by %s is already owned by %s",
                  trans_file_conflict_path(fc),
                  trans_file_conflict_requested_by(fc),
                  trans_file_conflict_owned_by(fc));
    }

    size_t blocked_count = trans_blocked_remove_count(trans);
    for (size_t i = 0; i < blocked_count; i++)
    {
        const struct trans_blocked_remove *b =
            trans_blocked_remove_at(trans, i);
        int dependent_count = trans_blocked_remove_dependent_count(b);

        char deps[512];
        deps[0] = '\0';
        for (int j = 0; j < dependent_count; j++)
        {
            strncat(deps, trans_blocked_remove_dependent_at(b, j),
                    sizeof(deps) - strlen(deps) - 1);
            if (j + 1 < dependent_count)
                strncat(deps, ", ", sizeof(deps) - strlen(deps) - 1);
        }
        ui_errorf("cannot remove %s: required by %s",
                  trans_blocked_remove_pkg_name(b), deps);
    }

    size_t held_count = trans_held_pkg_count(trans);
    for (size_t i = 0; i < held_count; i++)
    {
        const struct trans_held_pkg *h = trans_held_pkg_at(trans, i);
        ui_errorf("%s is held, %s blocked", trans_held_pkg_name(h),
                  op_label(trans_held_pkg_op(h)));
    }
}
