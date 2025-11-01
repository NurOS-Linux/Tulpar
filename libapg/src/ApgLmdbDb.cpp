// NurOS Ruzen42 2025
#include "apg_package/ApgLmdbDb.hpp"

#include <stdexcept>

LmdbDb::LmdbDb(const std::string& path, std::size_t mapSize, unsigned int maxDbs, int envFlags)
    : m_path(path), m_mapSize(mapSize), m_maxDbs(maxDbs), m_envFlags(envFlags) {}

LmdbDb::~LmdbDb()
{
    if (m_open) close();
}

void LmdbDb::open()
{
    if (m_open) return;
    m_env = lmdb::env::create();
    m_env.set_mapsize(m_mapSize);
    m_env.set_max_dbs(m_maxDbs);
    m_env.open(m_path.c_str(), m_envFlags, 0664);
    auto txn = lmdb::txn::begin(m_env);
    m_dbi = lmdb::dbi::open(txn, nullptr, 0);
    txn.commit();
    m_open = true;
}

void LmdbDb::close()
{
    if (!m_open) return;
    m_dbi.close(m_env);
    m_env.close();
    m_open = false;
}

void LmdbDb::createDatabase(const std::string& name, const unsigned int dbFlags) const
{
    ensureOpen();
    auto txn = lmdb::txn::begin(m_env);
    lmdb::dbi::open(txn, name.c_str(), dbFlags | MDB_CREATE);
    txn.commit();
}

bool LmdbDb::openDatabase(const std::string& name)
{
    ensureOpen();
    auto txn = lmdb::txn::begin(m_env, nullptr, MDB_RDONLY);
    try
    {
        m_dbi = lmdb::dbi::open(txn, name.c_str(), 0);
        txn.commit();
        return true;
    }
    catch (...)
    {
        txn.abort();
        return false;
    }
}

void LmdbDb::removeDatabase(const std::string& name) const {
    ensureOpen();
    auto txn = lmdb::txn::begin(m_env);
    auto dbi = lmdb::dbi::open(txn, name.c_str(), 0);
    dbi.drop(txn, true);
    txn.commit();
}

bool LmdbDb::put(const std::string& key, const std::string& value, bool overwrite)
{
    ensureOpen();
    auto txn = lmdb::txn::begin(m_env);
    try
    {
        m_dbi.put(txn, key, value, overwrite ? 0 : MDB_NOOVERWRITE);
        txn.commit();
        return true;
    }
    catch (...)
    {
        txn.abort();
        return false;
    }
}

std::optional<std::string> LmdbDb::get(const std::string& key) const
{
    ensureOpen();
    auto txn = lmdb::txn::begin(m_env, nullptr, MDB_RDONLY);
    std::string_view &value;
    bool found = false;
    try
    {
        m_dbi.get(txn, key, value);
        found = true;
    }
    catch (const lmdb::not_found_error&)
    {
        found = false;
    }
    txn.abort();
    if (found) return value.data();
    return std::nullopt;
}

bool LmdbDb::del(const std::string& key)
{
    ensureOpen();
    auto txn = lmdb::txn::begin(m_env);
    bool ok = true;
    try
    {
        m_dbi.del(txn, key);
        txn.commit();
    }
    catch (...)
    {
        txn.abort();
        ok = false;
    }
    return ok;
}

bool LmdbDb::contains(const std::string& key) const
{
    return get(key).has_value();
}

std::vector<std::string> LmdbDb::keys() const
{
    ensureOpen();
    auto txn = lmdb::txn::begin(m_env, nullptr, MDB_RDONLY);
    auto cursor = lmdb::cursor::open(txn, m_dbi);
    std::vector<std::string> result;
    std::string key, value;
    while (cursor.get(key, value, MDB_NEXT)) result.push_back(key);
    cursor.close();
    txn.abort();
    return result;
}

std::vector<std::pair<std::string, std::string>> LmdbDb::entries() const
{
    ensureOpen();
    auto txn = lmdb::txn::begin(m_env, nullptr, MDB_RDONLY);
    auto cursor = lmdb::cursor::open(txn, m_dbi);
    std::vector<std::pair<std::string, std::string>> result;
    std::string key, value;
    while (cursor.get(key, value, MDB_NEXT)) result.emplace_back(key, value);
    cursor.close();
    txn.abort();
    return result;
}

void LmdbDb::withReadTxn(const ReadTxnFn& fn) const
{
    ensureOpen();
    auto txn = lmdb::txn::begin(m_env, nullptr, MDB_RDONLY);
    fn(txn, m_dbi);
    txn.abort();
}

void LmdbDb::withWriteTxn(const WriteTxnFn& fn) const
{
    ensureOpen();
    auto txn = lmdb::txn::begin(m_env);
    fn(txn, m_dbi);
    txn.commit();
}

void LmdbDb::sync(const bool force)
{
    ensureOpen();
    m_env.sync(force);
}

const std::string& LmdbDb::path() const noexcept { return m_path; }
std::size_t LmdbDb::mapSize() const noexcept { return m_mapSize; }
unsigned int LmdbDb::maxDbs() const noexcept { return m_maxDbs; }
bool LmdbDb::isOpen() const noexcept { return m_open; }

void LmdbDb::ensureOpen() const
{
    if (!m_open) throw std::runtime_error("LmdbDb: environment not open");
}