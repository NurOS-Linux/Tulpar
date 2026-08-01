// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <apg/config.h>
#include <apg/sign.h>

#include "cmd_common.h"
#include "../cli/ui.h"
#include "../log/log.h"

void
cmd_print_usage(const char *usage)
{
    printf("usage: %s\n", usage);
}

struct db_handle *
cmd_open_db(const struct dest_ctx *dest, bool writable)
{
    ui_debugf("opening database at %s (%s, root %s)", dest->db_path,
              writable ? "writable" : "read-only", dest->root);

    if (writable && !dest_ctx_prepare_tree(dest))
    {
        ui_error("failed to prepare target root directory structure");
        return NULL;
    }

    struct db_handle *db =
        writable ? db_open(dest->db_path) : db_open_readonly(dest->db_path);

    if (!db)
        ui_errorf("failed to open package database at %s "
                  "(missing, or locked by another process)",
                  dest->db_path);

    return db;
}

void
cmd_warn_if_unsigned(const struct package *pkg)
{
    if (!pkg->pkg_path)
        return;

    char sig_path[4096];
    snprintf(sig_path, sizeof(sig_path), "%s.sig", pkg->pkg_path);

    struct stat st;
    if (stat(sig_path, &st) != 0)
    {
        ui_warnf("%s has no detached signature (%s); "
                 "installing without signature verification",
                 pkg->meta->name, sig_path);
        return;
    }

    if (!sign_verify(pkg->pkg_path, sig_path, false))
        ui_warnf("%s signature at %s could not be verified; "
                 "installing an unverified package",
                 pkg->meta->name, sig_path);
}

bool
cmd_run_transaction(struct apg_trans *trans, const struct dest_ctx *dest,
                    const struct tulpar_config *cfg, bool assume_yes,
                    bool require_signature_flag)
{
    install_policy policy = {
        .require_signature = require_signature_flag || cfg->require_signature,
        .keyring_dir = NULL,
        .backend = SIGN_BACKEND_SODIUM,
    };
    ui_debugf("policy: require_signature=%s",
              policy.require_signature ? "true" : "false");
    trans_set_policy(trans, &policy);

    ui_debug("preparing transaction (dependency resolution, conflict checks)");
    trans_error_t err = trans_prepare(trans);
    if (err != TRANS_OK)
    {
        switch (err)
        {
        case TRANS_ERR_CONFLICT:
        case TRANS_ERR_HAS_DEPENDENTS:
        case TRANS_ERR_FILE_CONFLICT:
        case TRANS_ERR_HELD:
            ui_print_conflicts(trans);
            break;
        case TRANS_ERR_CYCLE:
            ui_error("circular dependency detected");
            break;
        case TRANS_ERR_MISSING_DEP:
            ui_error("a required dependency could not be resolved");
            break;
        default:
            ui_error("failed to prepare the transaction");
            break;
        }
        return false;
    }

    ui_print_plan(trans);

    if (!ui_confirm("Proceed with this transaction?", assume_yes))
    {
        ui_info("aborted");
        return false;
    }

    if (!dest_ctx_prepare_tree(dest))
    {
        ui_error("failed to prepare target root directory structure");
        return false;
    }

    ui_debugf("committing transaction against root %s", dest->root);
    trans_error_t commit_err = trans_commit(trans, dest->root);
    if (commit_err != TRANS_OK)
    {
        switch (commit_err)
        {
        case TRANS_ERR_UNSIGNED:
            ui_error("a package failed signature verification and was "
                     "rejected by policy");
            break;
        case TRANS_ERR_INSTALL_FAILED:
            ui_error("installation failed; already-applied changes were "
                     "rolled back");
            break;
        default:
            ui_error("failed to commit the transaction");
            break;
        }
        log_write(TULPAR_LOG_ERROR, "transaction commit failed");
        return false;
    }

    ui_success("transaction complete");
    log_write(TULPAR_LOG_INFO, "transaction committed successfully");
    return true;
}
