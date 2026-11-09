// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <apg/config.h>
#include <apg/error.h>
#include <apg/sign.h>

#include "cmd_common.h"
#include "../cli/suggest.h"
#include "../cli/ui.h"
#include "../i18n.h"
#include "../log/log.h"
#include "../net/http.h"

void
cmd_print_usage(const char *usage)
{
    printf(_("usage: %s\n"), usage);
}

void
cmd_unknown_option(const char *arg, const char *usage)
{
    ui_errorf(_("unknown option: %s"), arg);

    if (strncmp(arg, "--", 2) == 0)
    {
        static const char *const globals[] = {"--verbose", "--quiet", "--color",
                                              "--help"};
        char names[48][64];
        const char *candidates[48];
        size_t count = 0;

        for (size_t i = 0; i < sizeof(globals) / sizeof(globals[0]); i++)
            candidates[count++] = globals[i];

        for (const char *p = strstr(usage, "--"); p && count < 48;
             p = strstr(p + 2, "--"))
        {
            size_t len = 2;
            while (p[len] == '-' || (p[len] >= 'a' && p[len] <= 'z') ||
                   (p[len] >= '0' && p[len] <= '9'))
                len++;
            if (len <= 2 || len >= sizeof(names[0]))
                continue;
            memcpy(names[count], p, len);
            names[count][len] = '\0';
            candidates[count] = names[count];
            count++;
        }

        char name[64];
        size_t len = strcspn(arg, "=");
        if (len >= sizeof(name))
            len = sizeof(name) - 1;
        memcpy(name, arg, len);
        name[len] = '\0';

        const char *best = suggest_closest(name, candidates, count);
        if (best)
            ui_hint(_("did you mean '%s'?"), best);
    }

    cmd_print_usage(usage);
}

const char *
cmd_libapg_reason(void)
{
    const char *err = apg_last_error();
    return err ? err : _("no further detail available");
}

struct package *
cmd_parse_package(const char *path)
{
    struct package *pkg = parse_package(path, NULL);
    if (!pkg)
        return NULL;

    char **files = NULL;
    int count = 0;
    if (!package_list_files(path, &files, &count))
    {
        package_free(pkg);
        return NULL;
    }

    str_list_free(&pkg->package_files);
    pkg->package_files.items = files;
    pkg->package_files.count = count;
    return pkg;
}

void
cmd_warn_repo_unreachable(const char *repo_url, bool using_cache)
{
    static char warned[32][512];
    static size_t warned_count = 0;
    static bool offline_noted = false;

    if (http_is_offline())
    {
        if (!offline_noted)
            ui_info(_("offline mode: remote repositories are not contacted"));
        offline_noted = true;
        return;
    }

    for (size_t i = 0; i < warned_count; i++)
        if (strcmp(warned[i], repo_url) == 0)
            return;
    if (warned_count < sizeof(warned) / sizeof(warned[0]))
        snprintf(warned[warned_count++], sizeof(warned[0]), "%s", repo_url);

    if (http_last_not_found() && using_cache)
        ui_warnf(_("repository %s no longer publishes a package index "
                   "(repodata.json); using the cached copy"),
                 repo_url);
    else if (http_last_not_found())
        ui_warnf(_("repository %s does not publish a package index "
                   "(repodata.json)"),
                 repo_url);
    else if (using_cache)
        ui_warnf(_("cannot refresh the index of %s, using the cached copy: "
                   "%s"),
                 repo_url, http_last_error());
    else
        ui_warnf(_("cannot reach repository %s: %s"), repo_url,
                 http_last_error());
}

void
cmd_download_progress(size_t now, size_t total, void *userdata)
{
    (void)userdata;
    ui_progress_update(now, total);
}

void
cmd_report_downloaded(const char *what, const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0)
    {
        ui_status(_("Downloaded"), "%s", what);
        return;
    }
    char size[32];
    ui_format_bytes((size_t)st.st_size, size, sizeof(size));
    ui_status(_("Downloaded"), "%s (%s)", what, size);
}

static bool
db_exists(const char *db_path)
{
    char data_path[4096];
    snprintf(data_path, sizeof(data_path), "%s/data.mdb", db_path);
    struct stat st;
    return stat(data_path, &st) == 0;
}

struct db_handle *
cmd_open_db(const struct dest_ctx *dest, bool writable)
{
    ui_debugf("opening database at %s (%s, root %s)", dest->db_path,
              writable ? "writable" : "read-only", dest->root);

    if (writable && !dest_ctx_prepare_tree(dest))
    {
        ui_error(_("failed to prepare target root directory structure"));
        return NULL;
    }

    if (!writable && !db_exists(dest->db_path))
    {
        ui_errorf(_("no package database at %s"), dest->db_path);
        ui_hint(_("nothing has been installed into %s yet"), dest->root);
        return NULL;
    }

    struct db_handle *db =
        writable ? db_open(dest->db_path) : db_open_readonly(dest->db_path);

    if (!db)
    {
        const char *reason = cmd_libapg_reason();
        ui_error(reason);
        if (strstr(reason, "locked by another process"))
            ui_hint(_("wait for the other package operation to finish, "
                      "then try again"));
    }

    return db;
}

static void
append_name(char *buf, size_t size, const char *name)
{
    size_t len = strlen(buf);
    snprintf(buf + len, size - len, "%s%s", len > 0 ? ", " : "", name);
}

void
cmd_warn_unsigned(struct package *const *pkgs, size_t count, bool required)
{
    if (required)
        return;

    char missing[1024] = "";
    char invalid[1024] = "";
    size_t missing_count = 0;
    size_t invalid_count = 0;

    for (size_t i = 0; i < count; i++)
    {
        const struct package *pkg = pkgs[i];
        if (!pkg->pkg_path)
            continue;

        char sig_path[4096];
        snprintf(sig_path, sizeof(sig_path), "%s.sig", pkg->pkg_path);

        struct stat st;
        if (stat(sig_path, &st) != 0)
        {
            ui_debugf("%s: no signature at %s", pkg->meta->name, sig_path);
            append_name(missing, sizeof(missing), pkg->meta->name);
            missing_count++;
        }
        else if (!sign_verify(pkg->pkg_path, sig_path, false))
        {
            ui_debugf("%s: signature at %s did not verify", pkg->meta->name,
                      sig_path);
            append_name(invalid, sizeof(invalid), pkg->meta->name);
            invalid_count++;
        }
    }

    if (missing_count > 0)
        ui_warnf(ngettext("%zu package has no signature and will be "
                          "installed unverified: %s",
                          "%zu packages have no signature and will be "
                          "installed unverified: %s",
                          missing_count),
                 missing_count, missing);
    if (invalid_count > 0)
        ui_warnf(ngettext("%zu package has a signature that could not be "
                          "verified: %s",
                          "%zu packages have signatures that could not be "
                          "verified: %s",
                          invalid_count),
                 invalid_count, invalid);
    if (missing_count > 0 || invalid_count > 0)
        ui_hint(_("pass --require-signature to refuse unverified packages"));
}

static void
report_step(const struct trans_step *step, size_t index, size_t total,
            void *userdata)
{
    (void)userdata;
    const char *verb;
    switch (trans_step_op(step))
    {
    case TRANS_OP_UPGRADE:
        verb = _("Upgrading");
        break;
    case TRANS_OP_REMOVE:
        verb = _("Removing");
        break;
    default:
        verb = _("Installing");
        break;
    }
    const char *version = trans_step_pkg_version(step);
    ui_status(verb, "[%zu/%zu] %s%s%s", index + 1, total,
              trans_step_pkg_name(step), version ? " " : "",
              version ? version : "");
}

double
cmd_seconds_since(const struct timespec *start)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)(now.tv_sec - start->tv_sec) +
           (double)(now.tv_nsec - start->tv_nsec) / 1e9;
}

static void
hint_if_isolation_failed(const char *reason)
{
    if (strstr(reason, "cannot isolate"))
        ui_hint(_("package scripts cannot be isolated on this system; set "
                  "require_script_sandbox = false in tulpar.conf to run them "
                  "without isolation"));
}

static bool
disk_space_acceptable(struct apg_trans *trans, const struct dest_ctx *dest,
                      bool assume_yes, bool dry_run)
{
    uint64_t needed = 0;
    uint64_t available = 0;
    if (!trans_estimate_space(trans, dest->root, &needed, &available))
    {
        ui_debugf("disk space estimate unavailable: %s", cmd_libapg_reason());
        return true;
    }

    ui_debugf("disk space: about %" PRIu64 " bytes needed, %" PRIu64
              " available",
              needed, available);
    if (needed <= available)
        return true;

    char needed_text[32];
    char available_text[32];
    ui_format_bytes((size_t)needed, needed_text, sizeof(needed_text));
    ui_format_bytes((size_t)available, available_text, sizeof(available_text));
    ui_warnf(_("the transaction needs about %s, but only %s is free on the "
               "target filesystem"),
             needed_text, available_text);
    ui_hint(_("the estimate is conservative; free some space, or continue if "
              "you expect it to be enough"));
    return dry_run || ui_confirm(_("Continue anyway?"), assume_yes);
}

bool
cmd_run_transaction(struct apg_trans *trans, struct db_handle *db,
                    const struct dest_ctx *dest,
                    const struct tulpar_config *cfg, bool assume_yes,
                    bool require_signature_flag, bool nodeps,
                    bool skip_dependents, bool dry_run)
{
    install_policy policy = {
        .require_signature = require_signature_flag || cfg->require_signature,
        .keyring_dir = NULL,
        .skip_dependency_check = nodeps,
        .skip_dependents_check = skip_dependents,
    };
    ui_debugf("policy: require_signature=%s, skip_dependency_check=%s, "
              "skip_dependents_check=%s, require_script_sandbox=%s",
              policy.require_signature ? "true" : "false",
              policy.skip_dependency_check ? "true" : "false",
              policy.skip_dependents_check ? "true" : "false",
              cfg->require_script_sandbox ? "true" : "false");
    trans_set_policy(trans, &policy);
    trans_set_dry_run(trans, dry_run);
    trans_set_require_sandbox(trans, cfg->require_script_sandbox);

    ui_debug("preparing transaction (dependency resolution, conflict checks)");
    trans_error_t err = trans_prepare(trans);
    if (err != TRANS_OK)
    {
        switch (err)
        {
        case TRANS_ERR_CONFLICT:
        case TRANS_ERR_FILE_CONFLICT:
            ui_print_conflicts(trans);
            break;
        case TRANS_ERR_HAS_DEPENDENTS:
            ui_print_conflicts(trans);
            ui_hint(_("remove the dependent packages too, or pass --nodeps "
                      "to remove anyway"));
            break;
        case TRANS_ERR_HELD:
            ui_print_conflicts(trans);
            ui_hint(_("release a held package with 'tulpar unhold <name>'"));
            break;
        case TRANS_ERR_CYCLE:
            ui_error(_("circular dependency detected"));
            break;
        case TRANS_ERR_MISSING_DEP:
            ui_errorf(_("cannot resolve dependencies: %s"),
                      cmd_libapg_reason());
            break;
        default:
            ui_errorf(_("failed to prepare the transaction: %s"),
                      cmd_libapg_reason());
            break;
        }
        return false;
    }

    ui_print_plan(trans, db);
    if (trans_plan_count(trans) == 0)
        return true;

    if (!disk_space_acceptable(trans, dest, assume_yes, dry_run))
    {
        ui_info(_("aborted"));
        return false;
    }

    if (!dry_run &&
        !ui_confirm(_("Proceed with this transaction?"), assume_yes))
    {
        ui_info(_("aborted"));
        return false;
    }

    if (!dest_ctx_prepare_tree(dest))
    {
        ui_error(_("failed to prepare target root directory structure"));
        return false;
    }

    ui_debugf("committing transaction against root %s%s", dest->root,
              dry_run ? " (dry run)" : "");
    if (!dry_run)
        trans_set_progress_cb(trans, report_step, NULL);
    struct timespec started;
    clock_gettime(CLOCK_MONOTONIC, &started);
    trans_error_t commit_err = trans_commit(trans, dest->root);
    if (commit_err != TRANS_OK)
    {
        switch (commit_err)
        {
        case TRANS_ERR_UNSIGNED:
            ui_errorf(_("signature verification failed: %s"),
                      cmd_libapg_reason());
            ui_hint(_("import the signing key with 'tulpar key add <file>'"));
            break;
        case TRANS_ERR_INSTALL_FAILED:
        {
            const char *reason = cmd_libapg_reason();
            ui_errorf(_("installation failed: %s"), reason);
            ui_hint(_("changes made by this transaction were rolled back"));
            hint_if_isolation_failed(reason);
            if (strstr(reason, strerror(ENOSPC)))
                ui_hint(_("free space on the target filesystem; packages are "
                          "unpacked into a temporary directory first, which "
                          "uses RAM when /tmp is a tmpfs"));
            break;
        }
        case TRANS_ERR_DB:
        {
            const char *reason = cmd_libapg_reason();
            ui_errorf(_("cannot record the installation: %s"), reason);
            ui_hint(_("changes made by this transaction were rolled back"));
            break;
        }
        case TRANS_ERR_REMOVE_FAILED:
        {
            const char *reason = cmd_libapg_reason();
            ui_errorf(_("removal failed: %s"), reason);
            ui_hint(_("changes made by this transaction were rolled back"));
            hint_if_isolation_failed(reason);
            break;
        }
        default:
            ui_errorf(_("failed to commit the transaction: %s"),
                      cmd_libapg_reason());
            break;
        }
        log_write(TULPAR_LOG_ERROR, "transaction commit failed");
        return false;
    }

    if (dry_run)
    {
        ui_status(_("Finished"), _("dry run; no changes were made"));
        return true;
    }

    ui_status(_("Finished"),
              ngettext("%zu operation in %.2fs", "%zu operations in %.2fs",
                       trans_plan_count(trans)),
              trans_plan_count(trans), cmd_seconds_since(&started));
    log_write(TULPAR_LOG_INFO, "transaction committed successfully");
    return true;
}
