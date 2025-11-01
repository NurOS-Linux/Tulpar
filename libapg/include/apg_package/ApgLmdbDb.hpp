// NurOS Ruzen42 2025
#ifndef APG_LMDB_DB_HPP
#define APG_LMDB_DB_HPP

#include <string>
#include <optional>
#include <vector>
#include <functional>
#include <lmdb++.h>

class LmdbDb {
public:
    using ReadTxnFn = std::function<void(const lmdb::txn&, const lmdb::dbi&)>;
    using WriteTxnFn = std::function<void(lmdb::txn&, const lmdb::dbi&)>;

    LmdbDb() = delete;
    explicit LmdbDb(const std::string& path, std::size_t mapSize = 1ULL << 30, unsigned int maxDbs = 16, int envFlags = 0);
    ~LmdbDb();

    void open();
    void close();

    void createDatabase(const std::string& name, unsigned int dbFlags = 0) const;
    bool openDatabase(const std::string& name);
    void removeDatabase(const std::string& name) const;

    bool put(const std::string& key, const std::string& value, bool overwrite = true);
    [[nodiscard]] std::optional<std::string> get(const std::string& key) const;
    bool del(const std::string& key);

    [[nodiscard]] bool contains(const std::string& key) const;
    [[nodiscard]] std::vector<std::string> keys() const;
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> entries() const;

    void withReadTxn(const ReadTxnFn& fn) const;
    void withWriteTxn(const WriteTxnFn& fn) const;

    void sync(bool force = false);

    [[nodiscard]] const std::string& path() const noexcept;
    [[nodiscard]] std::size_t mapSize() const noexcept;
    [[nodiscard]] unsigned int maxDbs() const noexcept;
    [[nodiscard]] bool isOpen() const noexcept;

private:
    void ensureOpen() const;
    std::string m_path;
    std::size_t m_mapSize;
    unsigned int m_maxDbs;
    int m_envFlags;
    bool m_open{false};
    lmdb::env m_env;
    lmdb::dbi m_dbi;
};

#endif // APG_LMDB_DB_HPP