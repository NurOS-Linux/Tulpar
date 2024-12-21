/**
 * @file tulpar.h
 * @brief Main header file for the Tulpar package manager
 * @author AnmiTaliDev
 * @date 2024-12-21
 * @version 0.1.0
 */

#ifndef TULPAR_H
#define TULPAR_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// Configuration paths
#define TULPAR_CONFIG_DIR "/etc/tulpar"
#define TULPAR_CACHE_DIR "/var/cache/tulpar"
#define TULPAR_DB_DIR "/var/lib/tulpar"
#define TULPAR_REPO_CONFIG "/etc/tulpar/apg/repo/notsystem"
#define TULPAR_LOG_FILE "/var/log/tulpar.log"

// Version information
#define TULPAR_VERSION "0.1.0"
#define TULPAR_VERSION_MAJOR 0
#define TULPAR_VERSION_MINOR 1
#define TULPAR_VERSION_PATCH 0

// Error codes
typedef enum {
    TULPAR_SUCCESS = 0,
    TULPAR_ERROR_INVALID_ARGS = -1,
    TULPAR_ERROR_PACKAGE_NOT_FOUND = -2,
    TULPAR_ERROR_DEPENDENCY_FAILED = -3,
    TULPAR_ERROR_DOWNLOAD_FAILED = -4,
    TULPAR_ERROR_INSTALL_FAILED = -5,
    TULPAR_ERROR_REMOVE_FAILED = -6,
    TULPAR_ERROR_UPDATE_FAILED = -7,
    TULPAR_ERROR_REPO_FAILED = -8,
    TULPAR_ERROR_NETWORK = -9,
    TULPAR_ERROR_PERMISSION = -10,
    TULPAR_ERROR_DISK_SPACE = -11,
    TULPAR_ERROR_CORRUPT_PACKAGE = -12,
    TULPAR_ERROR_INTERNAL = -99
} TulparError;

// Package dependency information
typedef struct {
    char *name;
    char *version;
    char *condition;  // ">", ">=", "=", "<", "<="
    bool optional;
} TulparDependency;

// Package metadata
typedef struct {
    char *name;
    char *version;
    char *description;
    char *maintainer;
    char *homepage;
    char *license;
    char *architecture;
    size_t installed_size;
    size_t download_size;
    time_t installation_date;
    TulparDependency *dependencies;
    size_t dependency_count;
    char **provides;
    size_t provides_count;
    char **conflicts;
    size_t conflicts_count;
    char **replaces;
    size_t replaces_count;
    char *checksum;
    char *signature;
} TulparPackage;

// Repository information
typedef struct {
    char *name;
    char *url;
    char *branch;
    bool enabled;
    time_t last_update;
    char *signature_key;
} TulparRepo;

// Package operation options
typedef struct {
    bool quiet;
    bool force;
    bool no_deps;
    bool no_scripts;
    bool download_only;
    bool assume_yes;
    bool verbose;
    bool debug;
} TulparOptions;

// Package database functions
TulparError tulpar_db_init(void);
TulparError tulpar_db_close(void);
TulparError tulpar_db_sync(void);
TulparPackage* tulpar_db_get_package(const char *name);
TulparError tulpar_db_add_package(const TulparPackage *package);
TulparError tulpar_db_remove_package(const char *name);
TulparError tulpar_db_update_package(const TulparPackage *package);

// Package management functions
TulparError tulpar_install_package(const char *package_name, const TulparOptions *options);
TulparError tulpar_remove_package(const char *package_name, const TulparOptions *options);
TulparError tulpar_update_package(const char *package_name, const TulparOptions *options);
TulparError tulpar_upgrade_all(const TulparOptions *options);
TulparError tulpar_local_install(const char *file_path, const TulparOptions *options);
TulparError tulpar_remote_install(const char *url, const TulparOptions *options);

// Repository management functions
TulparError tulpar_repo_add(const char *url);
TulparError tulpar_repo_remove(const char *url);
TulparError tulpar_repo_enable(const char *name);
TulparError tulpar_repo_disable(const char *name);
TulparError tulpar_repo_update(const char *name);
TulparError tulpar_repo_list(TulparRepo **repos, size_t *count);

// Query and search functions
TulparError tulpar_search_packages(const char *query, TulparPackage **results, size_t *count);
TulparError tulpar_list_installed(TulparPackage **packages, size_t *count);
TulparError tulpar_get_package_info(const char *name, TulparPackage *package);
TulparError tulpar_check_updates(TulparPackage **updates, size_t *count);

// Dependency resolution functions
TulparError tulpar_resolve_dependencies(const char *package_name, TulparDependency **deps, size_t *count);
TulparError tulpar_check_conflicts(const char *package_name, char **conflicts, size_t *count);
TulparError tulpar_verify_integrity(const char *package_name);

// Transaction handling
typedef struct TulparTransaction TulparTransaction;

TulparError tulpar_transaction_init(TulparTransaction **transaction);
TulparError tulpar_transaction_add(TulparTransaction *transaction, const char *package_name, bool remove);
TulparError tulpar_transaction_commit(TulparTransaction *transaction, const TulparOptions *options);
TulparError tulpar_transaction_abort(TulparTransaction *transaction);

// Configuration management
TulparError tulpar_config_load(void);
TulparError tulpar_config_save(void);
TulparError tulpar_config_get(const char *key, char *value, size_t size);
TulparError tulpar_config_set(const char *key, const char *value);

// Logging functions
typedef enum {
    TULPAR_LOG_DEBUG,
    TULPAR_LOG_INFO,
    TULPAR_LOG_WARNING,
    TULPAR_LOG_ERROR
} TulparLogLevel;

void tulpar_log_init(const char *log_file);
void tulpar_log_close(void);
void tulpar_log(TulparLogLevel level, const char *format, ...);

// Memory management
void tulpar_free_package(TulparPackage *package);
void tulpar_free_dependency(TulparDependency *dependency);
void tulpar_free_repo(TulparRepo *repo);
void tulpar_free_transaction(TulparTransaction *transaction);

// Utility functions
const char* tulpar_error_string(TulparError error);
bool tulpar_version_compare(const char *ver1, const char *ver2, const char *condition);
char* tulpar_get_arch(void);
bool tulpar_check_root(void);
char* tulpar_get_tmp_dir(void);
void tulpar_cleanup_tmp(void);

// Progress callback type
typedef void (*TulparProgressCallback)(const char *operation, 
                                     const char *package_name,
                                     int percentage,
                                     void *user_data);

// Set callback for progress reporting
void tulpar_set_progress_callback(TulparProgressCallback callback, void *user_data);

// Network functions
TulparError tulpar_download_file(const char *url, const char *destination);
TulparError tulpar_check_connectivity(void);

// Package verification
TulparError tulpar_verify_checksum(const char *file_path, const char *expected_checksum);
TulparError tulpar_verify_signature(const char *file_path, const char *signature_path);

// System information
typedef struct {
    char *os_name;
    char *os_version;
    char *kernel_version;
    char *architecture;
    size_t total_memory;
    size_t available_memory;
    size_t total_disk_space;
    size_t available_disk_space;
} TulparSystemInfo;

TulparError tulpar_get_system_info(TulparSystemInfo *info);
void tulpar_free_system_info(TulparSystemInfo *info);

#ifdef __cplusplus
}
#endif

#endif // TULPAR_H