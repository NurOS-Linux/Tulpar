/**
 * @file tulpar.c
 * @brief Tulpar Package Manager Core Implementation
 * @author AnmiTaliDev
 * @date 2024-12-23 19:51:00 UTC
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <curl/curl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <archive.h>
#include <archive_entry.h>
#include "tulpar.h"

#define TULPAR_DB "/var/lib/tulpar/packages.db"
#define TULPAR_CACHE "/var/cache/tulpar"
#define TULPAR_LOCK "/run/tulpar.lock"
#define TULPAR_REPO "https://repo.tulpar.org/packages"
#define TULPAR_BUF_SIZE (32 * 1024)  // 32KB buffer
#define TULPAR_VERSION "1.0.0"
#define TULPAR_MAX_RETRIES 3
#define TULPAR_TIMEOUT 30
#define APG_MAGIC "APG\x01"

typedef struct {
    char *url;
    char *dest; 
    char *sha256;
    size_t size;
    void *data;
} DownloadTask;

typedef struct {
    sqlite3 *db;
    CURLM *curl;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    FILE *logfp;
    DownloadTask *tasks;
    int task_count;
    int running;
    int verbose;
} TulparContext;

static TulparContext ctx = {0};

// Write callback for curl
static size_t write_cb(void *ptr, size_t size, size_t n, void *stream) {
    FILE *fp = (FILE*)stream;
    size_t written = fwrite(ptr, size, n, fp);
    fflush(fp);
    return written;
}

// Calculate SHA256 of file
static char* calc_sha256(const char *path) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;

    unsigned char buf[TULPAR_BUF_SIZE];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        SHA256_Update(&sha256, buf, n);
    }
    fclose(fp);

    SHA256_Final(hash, &sha256);

    char *result = malloc(SHA256_DIGEST_LENGTH * 2 + 1);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(result + i*2, "%02x", hash[i]);
    }
    return result;
}

// Download file with progress
static int download_file(const char *url, const char *dest, const char *sha256) {
    CURL *curl = curl_easy_init();
    if (!curl) return 1;

    FILE *fp = fopen(dest, "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, TULPAR_TIMEOUT);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Tulpar/" TULPAR_VERSION);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || http_code != 200) {
        unlink(dest);
        return 1;
    }

    // Verify SHA256 if provided
    if (sha256) {
        char *actual = calc_sha256(dest);
        if (!actual || strcmp(sha256, actual) != 0) {
            free(actual);
            unlink(dest);
            return 1;
        }
        free(actual);
    }

    return 0;
}

// Initialize tulpar
TulparError tulpar_init(void) {
    if (geteuid() != 0) {
        return TULPAR_ERR_PERM;
    }

    // Get lock
    int fd = open(TULPAR_LOCK, O_RDWR|O_CREAT, 0600);
    if (fd < 0 || flock(fd, LOCK_EX|LOCK_NB) < 0) {
        return TULPAR_ERR_LOCK;
    }

    // Create directories
    mkdir("/var/lib/tulpar", 0755);
    mkdir("/var/cache/tulpar", 0755);
    mkdir("/var/log/tulpar", 0755);

    // Open log
    ctx.logfp = fopen("/var/log/tulpar/tulpar.log", "a");

    // Initialize SQLite
    if (sqlite3_open(TULPAR_DB, &ctx.db)) {
        close(fd);
        return TULPAR_ERR_DB;
    }

    char *sql = 
        "CREATE TABLE IF NOT EXISTS packages ("
        "name TEXT PRIMARY KEY,"
        "version TEXT NOT NULL,"
        "size INTEGER,"
        "sha256 TEXT,"
        "install_date DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")";

    char *err = NULL;
    if (sqlite3_exec(ctx.db, sql, NULL, NULL, &err)) {
        sqlite3_free(err);
        sqlite3_close(ctx.db);
        close(fd);
        return TULPAR_ERR_DB;
    }

    // Initialize curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    ctx.curl = curl_multi_init();
    if (!ctx.curl) {
        sqlite3_close(ctx.db);
        close(fd);
        return TULPAR_ERR_INIT;
    }

    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);
    ctx.running = 1;

    return TULPAR_OK;
}

// Clean up tulpar
void tulpar_cleanup(void) {
    ctx.running = 0;
    pthread_cond_broadcast(&ctx.cond);

    if (ctx.db) sqlite3_close(ctx.db);
    if (ctx.curl) {
        curl_multi_cleanup(ctx.curl); 
        curl_global_cleanup();
    }
    if (ctx.logfp) fclose(ctx.logfp);

    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);

    unlink(TULPAR_LOCK);
}

// Install package
TulparError tulpar_install(const char *name, const TulparOptions *opts) {
    if (!name || !opts) return TULPAR_ERR_INVALID;

    pthread_mutex_lock(&ctx.mutex);

    // Check if already installed
    sqlite3_stmt *stmt;
    const char *sql = "SELECT version FROM packages WHERE name = ?";
    
    if (sqlite3_prepare_v2(ctx.db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        pthread_mutex_unlock(&ctx.mutex);
        return TULPAR_ERR_DB;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    int exists = (sqlite3_step(stmt) == SQLITE_ROW);
    const char *curr_ver = exists ? (const char*)sqlite3_column_text(stmt, 0) : NULL;
    sqlite3_finalize(stmt);

    if (exists && !opts->force) {
        pthread_mutex_unlock(&ctx.mutex);
        return TULPAR_OK;
    }

    // Download package
    char url[512], dest[256];
    snprintf(url, sizeof(url), "%s/%s/%s.apg", TULPAR_REPO, name, name);
    snprintf(dest, sizeof(dest), "%s/%s.apg", TULPAR_CACHE, name);

    int ret = download_file(url, dest, NULL);
    if (ret != 0) {
        pthread_mutex_unlock(&ctx.mutex);
        return TULPAR_ERR_NETWORK;
    }

    // Verify APG format
    FILE *fp = fopen(dest, "rb");
    if (!fp) {
        pthread_mutex_unlock(&ctx.mutex);
        return TULPAR_ERR_IO;
    }

    char magic[4];
    if (fread(magic, 1, 4, fp) != 4 || memcmp(magic, APG_MAGIC, 4) != 0) {
        fclose(fp);
        unlink(dest);
        pthread_mutex_unlock(&ctx.mutex);
        return TULPAR_ERR_INVALID;
    }
    fclose(fp);

    // Extract package
    struct archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    if (archive_read_open_filename(a, dest, TULPAR_BUF_SIZE) != ARCHIVE_OK) {
        archive_read_free(a);
        unlink(dest);
        pthread_mutex_unlock(&ctx.mutex);
        return TULPAR_ERR_EXTRACT;
    }

    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        archive_read_extract(a, entry, 0);
    }

    archive_read_close(a);
    archive_read_free(a);

    // Update database
    sql = "INSERT OR REPLACE INTO packages (name, version) VALUES (?, ?)";
    if (sqlite3_prepare_v2(ctx.db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        unlink(dest);
        pthread_mutex_unlock(&ctx.mutex);
        return TULPAR_ERR_DB;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, TULPAR_VERSION, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    unlink(dest);
    pthread_mutex_unlock(&ctx.mutex);
    
    return (rc == SQLITE_DONE) ? TULPAR_OK : TULPAR_ERR_DB;
}