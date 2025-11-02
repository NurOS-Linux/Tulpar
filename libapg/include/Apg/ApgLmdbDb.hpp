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
    LmdbDb(std::string path,
                    std::size_t mapSize = 10485760,
                    unsigned int maxDbs = 1,
                    int envFlags = 0);
	LmdbDb();

    void Close();

    bool Put(const std::string &key, const std::string &value, bool overwrite = true) const;
    std::optional<std::string> Get(const std::string &key) const;
    bool Delete(const std::string &key) const;

    [[nodiscard]] std::vector<std::string> Keys() const;
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> Entries() const;

private:
    std::string MPath;
    unsigned int MMapSize{};
    unsigned int MMaxDbs{};
    int MEnvFlags{};

    std::unique_ptr<lmdb::env> MEnv;
    std::unique_ptr<lmdb::dbi> MDbi;
};
#endif // APG_LMDB_DB_HPP