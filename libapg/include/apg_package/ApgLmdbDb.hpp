// NurOS Ruzen42 2025
#ifndef APG_LMDB_DB_HPP
#define APG_LMDB_DB_HPP

#pragma once
#include <lmdb++.h>
#include <optional>
#include <string>
#include <vector>

class LmdbDb
{
public:
    explicit LmdbDb(const std::string &path,
                    std::size_t mapSize = 10485760,
                    unsigned int maxDbs = 1,
                    int envFlags = 0);

    void close();

    [[nodiscard]] bool put(const std::string &key, const std::string &value, bool overwrite = true) const;
    [[nodiscard]] std::optional<std::string> get(const std::string &key) const;
    [[nodiscard]] bool del(const std::string &key) const;

    [[nodiscard]] std::vector<std::string> keys() const;
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> entries() const;

private:
    std::string m_path;
    std::size_t m_mapSize;
    unsigned int m_maxDbs;
    int m_envFlags;

    std::unique_ptr<lmdb::env> m_env;
    std::unique_ptr<lmdb::dbi> m_dbi;
};
#endif // APG_LMDB_DB_HPP