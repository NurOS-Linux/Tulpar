// NurOS Ruzen42 2025
#include "apg/ApgArchiver.hpp"
#include "archive.h"
#include "archive_entry.h"

bool ApgArchiver::Extract(const std::string &path, const std::string &destDir)
{
    archive *a = archive_read_new();
    archive *ext = archive_write_disk_new();
    archive_entry *entry;

    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM);
    archive_write_disk_set_standard_lookup(ext);

    if (archive_read_open_filename(a, path.c_str(), 10240) != ARCHIVE_OK)
    {
        archive_read_free(a);
        archive_write_free(ext);
        return false;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        const char *currentFile = archive_entry_pathname(entry);
        std::string fullOutputPath = destDir + "/" + currentFile;
        archive_entry_set_pathname(entry, fullOutputPath.c_str());

        if (archive_write_header(ext, entry) == ARCHIVE_OK)
        {
            const void *buff;
            size_t size;
            la_int64_t offset;

            while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK)
            {
                archive_write_data_block(ext, buff, size, offset);
            }
        }

        archive_write_finish_entry(ext);
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    return true;
}