// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "ui.h"
#include "../i18n.h"

#define COL_RESET "\x1b[0m"
#define COL_BOLD "\x1b[1m"
#define COL_DIM "\x1b[2m"
#define COL_RED "\x1b[31m"
#define COL_GREEN "\x1b[32m"
#define COL_YELLOW "\x1b[33m"
#define COL_BLUE "\x1b[34m"
#define COL_MAGENTA "\x1b[35m"
#define COL_CYAN "\x1b[36m"

#define STATUS_WIDTH 12
#define PROGRESS_REDRAW_NS 80000000L

static bool g_colors_enabled = false;
static bool g_colors_err = false;
static bool g_err_tty = false;
static bool g_verbose = false;
static bool g_quiet = false;
static const char *g_accent = COL_CYAN;

static char g_progress_verb[64];
static char g_progress_label[256];
static ui_progress_unit g_progress_unit;
static size_t g_progress_total = 0;
static size_t g_progress_current = 0;
static bool g_progress_active = false;
static bool g_progress_drawn = false;
static struct timespec g_progress_started;
static struct timespec g_progress_last_draw;

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

static size_t
text_width(const char *s)
{
    size_t width = 0;
    for (; *s; s++)
        if (((unsigned char)*s & 0xC0) != 0x80)
            width++;
    return width;
}

static void
print_verb(const char *verb)
{
    for (size_t w = text_width(verb); w < STATUS_WIDTH; w++)
        fputc(' ', stderr);
    if (g_colors_err)
        fprintf(stderr, "%s%s%s%s ", COL_BOLD, g_accent, verb, COL_RESET);
    else
        fprintf(stderr, "%s ", verb);
}

static void
print_truncated(const char *text, size_t max_width)
{
    size_t width = 0;
    for (const char *p = text; *p; p++)
    {
        bool starts_char = ((unsigned char)*p & 0xC0) != 0x80;
        if (starts_char && ++width > max_width)
            break;
        fputc(*p, stderr);
    }
}

bool
ui_parse_color_mode(const char *value, ui_color_mode *out)
{
    if (strcmp(value, "auto") == 0)
        *out = UI_COLOR_AUTO;
    else if (strcmp(value, "always") == 0)
        *out = UI_COLOR_ALWAYS;
    else if (strcmp(value, "never") == 0)
        *out = UI_COLOR_NEVER;
    else
        return false;
    return true;
}

static bool
env_set(const char *name)
{
    const char *v = getenv(name);
    return v && v[0] != '\0';
}

void
ui_init(const char *color_theme, ui_color_mode mode)
{
    g_accent = accent_from_theme(color_theme);
    g_err_tty = isatty(STDERR_FILENO) != 0;

    if (mode == UI_COLOR_AUTO)
    {
        if (env_set("NO_COLOR"))
            mode = UI_COLOR_NEVER;
        else if (env_set("CLICOLOR_FORCE"))
            mode = UI_COLOR_ALWAYS;
    }

    if (mode == UI_COLOR_NEVER)
    {
        g_colors_enabled = false;
        g_colors_err = false;
        return;
    }
    if (mode == UI_COLOR_ALWAYS)
    {
        g_colors_enabled = true;
        g_colors_err = true;
        return;
    }

    const char *term = getenv("TERM");
    bool dumb = term && strcmp(term, "dumb") == 0;
    g_colors_enabled = !dumb && isatty(STDOUT_FILENO) != 0;
    g_colors_err = !dumb && g_err_tty;
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
progress_clear(void)
{
    fflush(stdout);
    if (!g_progress_drawn)
        return;
    fputs("\r\x1b[2K", stderr);
    g_progress_drawn = false;
}

static void progress_draw(void);

static void
emit(FILE *stream, const char *color, const char *label, const char *msg)
{
    progress_clear();
    bool colored = stream == stderr ? g_colors_err : g_colors_enabled;
    if (colored)
        fprintf(stream, "%s%s%s%s %s\n", COL_BOLD, color, label, COL_RESET,
                msg);
    else
        fprintf(stream, "%s %s\n", label, msg);
    fflush(stream);
    if (g_progress_active)
        progress_draw();
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
    emit(stderr, COL_RED, _("error:"), msg);
}

void
ui_warn(const char *msg)
{
    if (g_quiet)
        return;
    emit(stderr, COL_YELLOW, _("warning:"), msg);
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
    emit(stderr, COL_BLUE, _("debug:"), msg);
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

void
ui_status(const char *verb, const char *fmt, ...)
{
    if (g_quiet)
        return;

    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    progress_clear();
    print_verb(verb);
    fprintf(stderr, "%s\n", buf);
    fflush(stderr);
    if (g_progress_active)
        progress_draw();
}

static void
emit_aside(const char *color, const char *label, const char *msg)
{
    progress_clear();
    if (g_colors_err)
        fprintf(stderr, "  %s%s%s%s %s\n", COL_BOLD, color, label, COL_RESET,
                msg);
    else
        fprintf(stderr, "  %s %s\n", label, msg);
    fflush(stderr);
}

void
ui_hint(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    emit_aside(COL_CYAN, _("help:"), buf);
}

void
ui_note(const char *fmt, ...)
{
    if (g_quiet)
        return;
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    emit_aside(COL_BLUE, _("note:"), buf);
}

void
ui_format_bytes(size_t bytes, char *buf, size_t size)
{
    static const char *const units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    double value = (double)bytes;
    size_t unit = 0;
    while (value >= 1024.0 && unit + 1 < sizeof(units) / sizeof(units[0]))
    {
        value /= 1024.0;
        unit++;
    }
    if (unit == 0)
        snprintf(buf, size, "%zu %s", bytes, units[0]);
    else
        snprintf(buf, size, "%.1f %s", value, units[unit]);
}

static bool
stdin_is_interactive(void)
{
    if (isatty(STDIN_FILENO) != 0)
        return true;
    ui_error(_("cannot ask for confirmation: standard input is not a "
               "terminal"));
    ui_hint(_("pass -y to proceed without a prompt"));
    return false;
}

bool
ui_confirm(const char *prompt, bool assume_yes)
{
    if (assume_yes)
        return true;
    if (!stdin_is_interactive())
        return false;

    progress_clear();
    if (g_colors_err)
        fprintf(stderr, "%s%s::%s %s %s ", COL_BOLD, g_accent, COL_RESET,
                prompt, _("[y/N]"));
    else
        fprintf(stderr, ":: %s %s ", prompt, _("[y/N]"));
    fflush(stderr);

    char line[16];
    if (!fgets(line, sizeof(line), stdin))
        return false;

    return line[0] == 'y' || line[0] == 'Y';
}

int
ui_select(const char *prompt, const char *const *options, int count,
          bool assume_yes)
{
    if (assume_yes || isatty(STDIN_FILENO) == 0)
        return -1;

    progress_clear();
    if (g_colors_err)
        fprintf(stderr, "%s%s::%s %s\n", COL_BOLD, g_accent, COL_RESET, prompt);
    else
        fprintf(stderr, ":: %s\n", prompt);

    for (int i = 0; i < count; i++)
        fprintf(stderr, "  %d) %s\n", i + 1, options[i]);

    for (int attempt = 0; attempt < 3; attempt++)
    {
        fprintf(stderr, _("enter a number [1-%d]: "), count);
        fflush(stderr);

        char line[32];
        if (!fgets(line, sizeof(line), stdin))
            return -1;

        int choice = atoi(line);
        if (choice >= 1 && choice <= count)
            return choice - 1;

        fprintf(stderr, _("invalid choice\n"));
    }

    return -1;
}

static long
elapsed_ns(const struct timespec *from, const struct timespec *to)
{
    return (long)(to->tv_sec - from->tv_sec) * 1000000000L +
           (to->tv_nsec - from->tv_nsec);
}

static int
terminal_width(void)
{
    struct winsize ws;
    if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        return ws.ws_col;
    return 80;
}

static void
format_amount(size_t value, char *buf, size_t size)
{
    if (g_progress_unit == UI_PROGRESS_BYTES)
        ui_format_bytes(value, buf, size);
    else
        snprintf(buf, size, "%zu", value);
}

static void
progress_draw(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    g_progress_last_draw = now;

    char current[32];
    char total[32];
    char counts[96];
    format_amount(g_progress_current, current, sizeof(current));
    if (g_progress_total > 0)
    {
        format_amount(g_progress_total, total, sizeof(total));
        snprintf(counts, sizeof(counts), "%s/%s", current, total);
    }
    else
        snprintf(counts, sizeof(counts), "%s", current);

    char rate[48] = "";
    long ns = elapsed_ns(&g_progress_started, &now);
    if (g_progress_unit == UI_PROGRESS_BYTES && ns > 0)
    {
        char per_sec[32];
        ui_format_bytes((size_t)((double)g_progress_current * 1e9 / (double)ns),
                        per_sec, sizeof(per_sec));
        snprintf(rate, sizeof(rate), " %s/s", per_sec);
    }

    int width = terminal_width();
    size_t label_width = text_width(g_progress_label);
    if (label_width > 24)
        label_width = 24;
    int bar_width = width - STATUS_WIDTH - (int)strlen(counts) -
                    (int)strlen(rate) - (int)label_width - 7;
    if (bar_width > 40)
        bar_width = 40;

    fputs("\r\x1b[2K", stderr);
    print_verb(g_progress_verb);

    if (g_progress_total > 0 && bar_width >= 10)
    {
        size_t cur = g_progress_current < g_progress_total ? g_progress_current
                                                           : g_progress_total;
        int filled = (int)((double)cur / (double)g_progress_total * bar_width);
        fputc('[', stderr);
        for (int i = 0; i < bar_width; i++)
        {
            if (i < filled)
                fputc('=', stderr);
            else if (i == filled)
                fputc('>', stderr);
            else
                fputc(' ', stderr);
        }
        fputs("] ", stderr);
    }

    fprintf(stderr, "%s%s", counts, rate);
    int used = STATUS_WIDTH + 1 + (bar_width >= 10 ? bar_width + 3 : 0) +
               (int)strlen(counts) + (int)strlen(rate);
    int room = width - used - 3;
    if (g_progress_label[0] != '\0' && room > 4)
    {
        if (g_colors_err)
        {
            fprintf(stderr, "  %s", COL_DIM);
            print_truncated(g_progress_label, (size_t)room);
            fputs(COL_RESET, stderr);
        }
        else
        {
            fputs("  ", stderr);
            print_truncated(g_progress_label, (size_t)room);
        }
    }
    fflush(stderr);
    g_progress_drawn = true;
}

void
ui_progress_start(const char *verb, const char *label, size_t total,
                  ui_progress_unit unit)
{
    snprintf(g_progress_verb, sizeof(g_progress_verb), "%s", verb);
    snprintf(g_progress_label, sizeof(g_progress_label), "%s",
             label ? label : "");
    g_progress_unit = unit;
    g_progress_total = total;
    g_progress_current = 0;
    g_progress_active = g_err_tty && !g_quiet;
    g_progress_drawn = false;
    clock_gettime(CLOCK_MONOTONIC, &g_progress_started);

    if (g_progress_active)
        progress_draw();
}

void
ui_progress_update(size_t current, size_t total)
{
    g_progress_current = current;
    if (total > 0)
        g_progress_total = total;
    if (!g_progress_active)
        return;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (elapsed_ns(&g_progress_last_draw, &now) < PROGRESS_REDRAW_NS)
        return;
    progress_draw();
}

void
ui_progress_finish(void)
{
    progress_clear();
    fflush(stderr);
    g_progress_active = false;
}

static const char *
op_label(trans_op_t op)
{
    switch (op)
    {
    case TRANS_OP_INSTALL:
        return _("install");
    case TRANS_OP_UPGRADE:
        return _("upgrade");
    case TRANS_OP_REMOVE:
        return _("remove");
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

static void
print_padded(const char *color, const char *text, size_t width)
{
    if (g_colors_enabled && color)
        printf("%s%s%s", color, text, COL_RESET);
    else
        fputs(text, stdout);
    for (size_t w = text_width(text); w < width; w++)
        putchar(' ');
}

static char *
installed_version(struct db_handle *db, const char *name)
{
    if (!db)
        return NULL;
    struct package *pkg = db_get(db, name);
    if (!pkg)
        return NULL;
    char *version = pkg->meta->version ? strdup(pkg->meta->version) : NULL;
    package_free(pkg);
    return version;
}

void
ui_print_plan(const struct apg_trans *trans, struct db_handle *db)
{
    size_t count = trans_plan_count(trans);

    if (count == 0)
    {
        ui_info(_("nothing to do"));
        return;
    }
    if (g_quiet)
        return;

    size_t op_width = 0;
    size_t name_width = 0;
    size_t per_op[3] = {0, 0, 0};
    for (size_t i = 0; i < count; i++)
    {
        const struct trans_step *s = trans_plan_at(trans, i);
        size_t w = text_width(op_label(trans_step_op(s)));
        if (w > op_width)
            op_width = w;
        w = text_width(trans_step_pkg_name(s));
        if (w > name_width)
            name_width = w;
        switch (trans_step_op(s))
        {
        case TRANS_OP_INSTALL:
            per_op[0]++;
            break;
        case TRANS_OP_UPGRADE:
            per_op[1]++;
            break;
        case TRANS_OP_REMOVE:
            per_op[2]++;
            break;
        }
    }

    printf("\n");
    if (g_colors_enabled)
        printf("%s%s (%zu)%s\n", COL_BOLD, _("Packages"), count, COL_RESET);
    else
        printf("%s (%zu)\n", _("Packages"), count);

    for (size_t i = 0; i < count; i++)
    {
        const struct trans_step *s = trans_plan_at(trans, i);
        trans_op_t op = trans_step_op(s);
        const char *name = trans_step_pkg_name(s);
        const char *ver = trans_step_pkg_version(s);

        fputs("  ", stdout);
        print_padded(op_color(op), op_label(op), op_width + 2);
        print_padded(COL_BOLD, name, name_width + 2);

        char *old = op == TRANS_OP_INSTALL ? NULL : installed_version(db, name);
        if (op == TRANS_OP_UPGRADE && old)
        {
            if (g_colors_enabled)
                printf("%s%s%s -> %s", COL_DIM, old, COL_RESET, ver ? ver : "");
            else
                printf("%s -> %s", old, ver ? ver : "");
        }
        else if (op == TRANS_OP_REMOVE)
            fputs(old ? old : "", stdout);
        else
            fputs(ver ? ver : "", stdout);
        free(old);
        putchar('\n');
    }

    const char *parts[3];
    char buf[3][64];
    size_t part_count = 0;
    if (per_op[0] > 0)
    {
        snprintf(buf[part_count], sizeof(buf[0]), _("%zu to install"),
                 per_op[0]);
        parts[part_count] = buf[part_count];
        part_count++;
    }
    if (per_op[1] > 0)
    {
        snprintf(buf[part_count], sizeof(buf[0]), _("%zu to upgrade"),
                 per_op[1]);
        parts[part_count] = buf[part_count];
        part_count++;
    }
    if (per_op[2] > 0)
    {
        snprintf(buf[part_count], sizeof(buf[0]), _("%zu to remove"),
                 per_op[2]);
        parts[part_count] = buf[part_count];
        part_count++;
    }

    printf("\n  ");
    for (size_t i = 0; i < part_count; i++)
        printf("%s%s", i > 0 ? ", " : "", parts[i]);
    printf("\n\n");
}

void
ui_print_conflicts(const struct apg_trans *trans)
{
    size_t conflict_count = trans_conflict_count(trans);
    for (size_t i = 0; i < conflict_count; i++)
    {
        const struct trans_conflict *c = trans_conflict_at(trans, i);
        ui_errorf(_("%s conflicts with installed package %s"),
                  trans_conflict_pkg_name(c), trans_conflict_conflicts_with(c));
    }

    size_t file_conflict_count = trans_file_conflict_count(trans);
    for (size_t i = 0; i < file_conflict_count; i++)
    {
        const struct trans_file_conflict *fc = trans_file_conflict_at(trans, i);
        ui_errorf(_("file %s requested by %s is already owned by %s"),
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
        ui_errorf(_("cannot remove %s: required by %s"),
                  trans_blocked_remove_pkg_name(b), deps);
    }

    size_t held_count = trans_held_pkg_count(trans);
    for (size_t i = 0; i < held_count; i++)
    {
        const struct trans_held_pkg *h = trans_held_pkg_at(trans, i);
        ui_errorf(_("%s is held, %s blocked"), trans_held_pkg_name(h),
                  op_label(trans_held_pkg_op(h)));
    }
}
