// NurOS Ruzen42 2025
#include <filesystem>
#include <system_error>
#include <utility>

#include "Apg/ApgLmdbDb.hpp"
#include "Apg/Logger.hpp"

LmdbDb::LmdbDb(std::string path, const std::size_t mapSize, const unsigned int maxDbs, const int envFlags)
    : MPath(std::move(path)), MMapSize(mapSize), MMaxDbs(maxDbs), MEnvFlags(envFlags)
{
    if (const std::filesystem::path dbPath(MPath); !std::filesystem::exists(dbPath))
    {
        if (std::error_code ec; !std::filesystem::create_directories(dbPath, ec))
        {
            Logger::LogError("Failed to create database directory: " + MPath + " - " + ec.message());
            throw std::runtime_error("Failed to create database directory: " + ec.message());
        }
        Logger::LogDebug("Created database directory: " + MPath);
    }

    MEnv = std::make_unique<lmdb::env>(lmdb::env::create());
    MEnv->set_mapsize(MMapSize);
    MEnv->set_max_dbs(MMaxDbs);
    MEnv->open(MPath.c_str(), MEnvFlags, 0664);

    auto txn = lmdb::txn::begin(*MEnv);
    MDbi = std::make_unique<lmdb::dbi>(lmdb::dbi::open(txn, nullptr, MDB_CREATE));
    txn.commit();
}

LmdbDb::LmdbDb() {}

void LmdbDb::Close()
{
    if (MEnv)
    {
        MEnv->close();
        MEnv.reset();
        MDbi.reset();
    }
    Logger::LogDebug("Database closed");
}

bool LmdbDb::Put(const std::string &key, const std::string &value, const bool overwrite) const
{
    auto txn = lmdb::txn::begin(*MEnv);
    try
    {
        if (overwrite)
        {
            MDbi->put(txn, key, value);
        }
        else
        {
            if (std::string_view val; !MDbi->get(txn, key, val))
            {
                MDbi->put(txn, key, value);
            }
        }
        txn.commit();
        return true;
    }
    catch (...)
    {
        txn.abort();
        return false;
    }
}

std::optional<std::string> LmdbDb::Get(const std::string &key) const
{
    auto txn = lmdb::txn::begin(*MEnv, nullptr, MDB_RDONLY);
    std::string_view value;
    const bool found = MDbi->get(txn, key, value);
    txn.abort();

    if (!found)
    {
        return std::nullopt;
    }

    return std::string(value);
}

bool LmdbDb::Delete(const std::string &key) const
{
    auto txn = lmdb::txn::begin(*MEnv);
    bool success = false;
    try
    {
        success = MDbi->del(txn, key);
        txn.commit();
    }
    catch (...)
    {
        txn.abort();
        success = false;
    }
    return success;
}

std::vector<std::string> LmdbDb::Keys() const
{
    std::vector<std::string> result;
    auto txn = lmdb::txn::begin(*MEnv, nullptr, MDB_RDONLY);
    auto cursor = lmdb::cursor::open(txn, *MDbi);

    std::string_view key, value;
    while (cursor.get(key, value, MDB_NEXT))
    {
        result.emplace_back(key);
    }

    cursor.close();
    txn.abort();
    return result;
}

std::vector<std::pair<std::string, std::string>> LmdbDb::Entries() const
{
    std::vector<std::pair<std::string, std::string>> result;
    auto txn = lmdb::txn::begin(*MEnv, nullptr, MDB_RDONLY);
    auto cursor = lmdb::cursor::open(txn, *MDbi);

    std::string_view key, value;
    while (cursor.get(key, value, MDB_NEXT))
    {
        result.emplace_back(std::string(key), std::string(value));
    }

    cursor.close();
    txn.abort();
    return result;
}