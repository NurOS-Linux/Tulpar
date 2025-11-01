// NurOS Ruzen42 2025
#include <utility>

#include "apg/ApgLmdbDb.hpp"

LmdbDb::LmdbDb(std::string path, const std::size_t mapSize, const unsigned int maxDbs, const int envFlags)
    : m_path(std::move(path)), m_mapSize(mapSize), m_maxDbs(maxDbs), m_envFlags(envFlags)
{
    m_env = std::make_unique<lmdb::env>(lmdb::env::create());
    m_env->set_mapsize(m_mapSize);
    m_env->set_max_dbs(m_maxDbs);
    m_env->open(m_path.c_str(), m_envFlags, 0664);

    auto txn = lmdb::txn::begin(*m_env);
    m_dbi = std::make_unique<lmdb::dbi>(lmdb::dbi::open(txn, nullptr, MDB_CREATE));
    txn.commit();
}

void LmdbDb::close()
{
    if (m_env)
    {
        m_env->close();
        m_env.reset();
        m_dbi.reset();
    }
}

bool LmdbDb::put(const std::string &key, const std::string &value, const bool overwrite) const
{
    auto txn = lmdb::txn::begin(*m_env);
    try
    {
        if (overwrite)
        {
            m_dbi->put(txn, key, value);
        }
        else
        {
            if (std::string_view val; !m_dbi->get(txn, key, val))
            {
                m_dbi->put(txn, key, value);
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

std::optional<std::string> LmdbDb::get(const std::string &key) const
{
    auto txn = lmdb::txn::begin(*m_env, nullptr, MDB_RDONLY);
    std::string_view value;
    const bool found = m_dbi->get(txn, key, value);
    txn.abort();

    if (!found)
    {
        return std::nullopt;
    }

    return std::string(value);
}

bool LmdbDb::del(const std::string &key) const
{
    auto txn = lmdb::txn::begin(*m_env);
    bool success = false;
    try
    {
        success = m_dbi->del(txn, key);
        txn.commit();
    }
    catch (...)
    {
        txn.abort();
        success = false;
    }
    return success;
}

std::vector<std::string> LmdbDb::keys() const
{
    std::vector<std::string> result;
    auto txn = lmdb::txn::begin(*m_env, nullptr, MDB_RDONLY);
    auto cursor = lmdb::cursor::open(txn, *m_dbi);

    std::string_view key, value;
    while (cursor.get(key, value, MDB_NEXT))
    {
        result.emplace_back(key);
    }

    cursor.close();
    txn.abort();
    return result;
}

std::vector<std::pair<std::string, std::string>> LmdbDb::entries() const
{
    std::vector<std::pair<std::string, std::string>> result;
    auto txn = lmdb::txn::begin(*m_env, nullptr, MDB_RDONLY);
    auto cursor = lmdb::cursor::open(txn, *m_dbi);

    std::string_view key, value;
    while (cursor.get(key, value, MDB_NEXT))
    {
        result.emplace_back(std::string(key), std::string(value));
    }

    cursor.close();
    txn.abort();
    return result;
}