// NurOS Ruzen42 2025 apg/archive.c
// Last change: Dec 21

#include <archive.h>
#include <archive_entry.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <iron/logger.h>

bool
extract_to_dir(const char *archive_path, const char *path_dest)
{
    struct archive *a;
    struct archive *ext;
    struct archive_entry *entry;
    int r;
    char full_path[PATH_MAX];

    a = archive_read_new();
    archive_read_support_filter_xz(a);
    archive_read_support_format_tar(a);

    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM);

    if (archive_read_open_filename(a, archive_path, 10240) != ARCHIVE_OK) {
        return 0;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        snprintf(full_path, sizeof(full_path), "%s/%s", path_dest, archive_entry_pathname(entry));
        archive_entry_set_pathname(entry, full_path);

        r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
            fprintf(stderr, "Header error: %s\n", archive_error_string(ext));
        } else {
            const void *buff;
            size_t size;
            la_int64_t offset;

            while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                if (archive_write_data_block(ext, buff, size, offset) != ARCHIVE_OK) {
                    fprintf(stderr, "Error while writing data: %s\n", archive_error_string(ext));
                    break;
                }
            }
        }
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    return 1;
}

bool
unarchive_package(const struct package *pkg, const char *path)
{
    if (!extract_to_dir(pkg->pkg_path, path)) {
        log_error("Failed to extract package");
        return false;
    }
    log_info("Package extracted successfully");
    return true;
}

