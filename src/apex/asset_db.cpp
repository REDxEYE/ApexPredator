#include "apex/asset_db.h"
#include <algorithm>
#include <bit>
#include <format>
#include "utils/murmur3.h"
#include <stdexcept>

#include "redscore/platform/logger.h"

namespace {
    AssetDB *instance = nullptr;
    int64 sql_hash(const uint64 hash) { return std::bit_cast<int64>(hash); }


    constexpr auto kv_schema =
            "CREATE TABLE kv (lookup3 INTEGER NOT NULL, murmur INTEGER NOT NULL DEFAULT 0,"
            "v TEXT NOT NULL, PRIMARY KEY(lookup3,murmur)) WITHOUT ROWID;"
            "CREATE INDEX idx_kv_murmur ON kv(murmur) WHERE murmur<>0;";

    void create_files_table(SQLite::Database &db, const std::string_view table) {
        db.exec(std::format(
            "CREATE TABLE IF NOT EXISTS {} (lookup3 INTEGER NOT NULL, murmur INTEGER NOT NULL DEFAULT 0,"
            "name TEXT, size INTEGER NOT NULL, parent INTEGER NOT NULL DEFAULT 0,"
            "PRIMARY KEY(lookup3,murmur)) WITHOUT ROWID;"
            "CREATE INDEX IF NOT EXISTS idx_{}_murmur ON {}(murmur) WHERE murmur<>0;"
            "CREATE INDEX IF NOT EXISTS idx_{}_parent ON {}(parent);",
            table, table, table, table, table));
    }

    bool table_exists(SQLite::Database &db, const std::string_view table) {
        SQLite::Statement query(db, "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?");
        query.bind(1, std::string(table));
        return query.executeStep();
    }
}

void AssetDB::set_instance(AssetDB *db) { instance = db; }

AssetDB *AssetDB::get_instance() {
    if (!instance) throw std::runtime_error("AssetDB: instance is not set");
    return instance;
}

void AssetDB::init_new() {
    m_db.exec(kv_schema);
    create_files_table(m_db, m_files_table);
    m_db.exec("PRAGMA user_version=2;");
}

void AssetDB::migrate() {
    bool legacy = false;
    {
        SQLite::Statement columns(m_db, "PRAGMA table_info(kv)");
        while (columns.executeStep())
            if (columns.getColumn(1).getString() == "k") legacy = true;
    }
    if (!legacy) {
        SQLite::Statement version(m_db, "PRAGMA user_version");
        if (version.executeStep() && version.getColumn(0).getInt() >= 2) {
            if (table_exists(m_db, "files"))
                throw std::runtime_error("Unified database still contains an unassigned files table");
            create_files_table(m_db, m_files_table);
            return;
        }
        if (!table_exists(m_db, "files"))
            throw std::runtime_error("Database has no legacy files table");
        if (table_exists(m_db, m_files_table))
            throw std::runtime_error("Database has both legacy and game files tables");
        version.reset();
        SQLite::Transaction transaction(m_db);
        // Existing dual-hash databases stored zero MMH3 even for known names.
        // Advance by lookup3 while changing the row's murmur key in place.
        const auto backfill = [&](const char *next_sql, const char *target_sql,
                                  const char *update_sql, const char *remove_sql) {
            SQLite::Statement next(m_db, next_sql), target(m_db, target_sql);
            SQLite::Statement update(m_db, update_sql), remove(m_db, remove_sql);
            int64 last = -1;
            for (;;) {
                next.reset();
                next.bind(1, last);
                if (!next.executeStep()) break;
                const int64 lookup3 = next.getColumn(0).getInt64();
                const auto &column = next.getColumn(1);
                const std::string_view name(column.getText(), column.getBytes());
                const uint64 murmur = MurmurHash3_x64_128(name.data(), name.size(), 0);
                if (!murmur) throw std::runtime_error("Named hash has reserved zero Murmur value");
                target.reset();
                target.bind(1, lookup3);
                target.bind(2, sql_hash(murmur));
                const bool duplicate = target.executeStep();
                if (duplicate) {
                    const auto &stored = target.getColumn(0);
                    if (std::string_view(stored.getText(), stored.getBytes()) != name)
                        throw std::runtime_error("Different names have identical Lookup3 and Murmur hashes");
                }
                target.reset();
                next.reset();
                if (duplicate) {
                    remove.reset();
                    remove.bind(1, lookup3);
                    remove.exec();
                } else {
                    update.reset();
                    update.bind(1, sql_hash(murmur));
                    update.bind(2, lookup3);
                    update.exec();
                }
                last = lookup3;
            }
        };
        backfill("SELECT lookup3,v FROM kv WHERE murmur=0 AND v<>'' AND lookup3>?1 ORDER BY lookup3 LIMIT 1",
                 "SELECT v FROM kv WHERE lookup3=?1 AND murmur=?2",
                 "UPDATE kv SET murmur=?1 WHERE lookup3=?2 AND murmur=0",
                 "DELETE FROM kv WHERE lookup3=?1 AND murmur=0");
        backfill("SELECT lookup3,name FROM files WHERE murmur=0 AND name<>'' AND lookup3>?1 ORDER BY lookup3 LIMIT 1",
                 "SELECT name FROM files WHERE lookup3=?1 AND murmur=?2",
                 "UPDATE files SET murmur=?1 WHERE lookup3=?2 AND murmur=0",
                 "DELETE FROM files WHERE lookup3=?1 AND murmur=0");
        m_db.exec("CREATE INDEX IF NOT EXISTS idx_kv_murmur ON kv(murmur) WHERE murmur<>0;");
        m_db.exec("DROP INDEX IF EXISTS idx_files_murmur; DROP INDEX IF EXISTS idx_files_parent;");
        m_db.exec(std::format("ALTER TABLE files RENAME TO {};", m_files_table));
        create_files_table(m_db, m_files_table);
        m_db.exec("PRAGMA user_version=2;");
        transaction.commit();
        return;
    }
    if (!table_exists(m_db, "files"))
        throw std::runtime_error("Legacy database has no files table");
    SQLite::Transaction transaction(m_db);
    m_db.exec("ALTER TABLE kv RENAME TO kv_legacy; ALTER TABLE files RENAME TO files_legacy;"
              "DROP INDEX IF EXISTS idx_files_murmur; DROP INDEX IF EXISTS idx_files_parent;");
    init_new();
    const auto domain = legacy_hash_domain();
    auto hashes_for = [domain](const uint64 key, const std::string &name) {
        auto hashes = name.empty() ? StringHashes{0, 0} : string_hashes(name);
        if (domain == LegacyHashDomain::Lookup3)
            hashes.lookup3 = static_cast<uint32>(key);
        else
            hashes.murmur = key;
        return hashes;
    };
    {
        SQLite::Statement rows(m_db, "SELECT k,v FROM kv_legacy");
        SQLite::Statement insert(m_db, "INSERT INTO kv VALUES(?,?,?)");
        while (rows.executeStep()) {
            const auto name = rows.getColumn(1).getString();
            const auto hashes = hashes_for(static_cast<uint64>(rows.getColumn(0).getInt64()), name);
            insert.reset();
            insert.bind(1, int64(hashes.lookup3));
            insert.bind(2, sql_hash(hashes.murmur));
            insert.bind(3, name);
            insert.exec();
        }
    }
    {
        SQLite::Statement rows(m_db, "SELECT hash,name,size,parent FROM files_legacy");
        SQLite::Statement insert(m_db, std::format("INSERT INTO {} VALUES(?,?,?,?,?)", m_files_table));
        while (rows.executeStep()) {
            auto hashes = hashes_for(static_cast<uint64>(rows.getColumn(0).getInt64()),
                                     rows.getColumn(1).isNull() ? "" : rows.getColumn(1).getString());
            insert.reset();
            insert.bind(1, int64(hashes.lookup3));
            insert.bind(2, sql_hash(hashes.murmur));
            if (rows.getColumn(1).isNull()) insert.bind(3);
            else insert.bind(3, rows.getColumn(1).getString());
            insert.bind(4, rows.getColumn(2).getInt64());
            insert.bind(5, rows.getColumn(3).getInt64());
            insert.exec();
        }
    }
    m_db.exec("DROP TABLE kv_legacy; DROP TABLE files_legacy;");
    transaction.commit();
}

void AssetDB::kv_put(const StringHashes hashes, const std::string_view value) const {
    auto &existing = *m_kv_pair;
    existing.reset();
    existing.bind(1, static_cast<int64>(hashes.lookup3));
    existing.bind(2, sql_hash(hashes.murmur));
    if (existing.executeStep()) {
        const auto &column = existing.getColumn(0);
        if (std::string_view(column.getText(), column.getBytes()) != value)
            throw std::runtime_error("Different names have identical Lookup3 and Murmur hashes");
        return;
    }
    auto &s = *m_kv_put;
    s.reset();
    s.bind(1, static_cast<int64>(hashes.lookup3));
    s.bind(2, sql_hash(hashes.murmur));
    s.bind(3, std::string(value));
    s.exec();
}

std::optional<std::string> AssetDB::kv_get(const uint64 key, const HashType type) const {
    SQLite::Statement s(m_db, "SELECT v FROM kv WHERE lookup3=?1 OR (murmur<>0 AND murmur=?1) LIMIT 2");
    s.bind(1, sql_hash(key));
    if (!s.executeStep()) return std::nullopt;
    auto value = s.getColumn(0).getString();
    if (s.executeStep()) return std::nullopt;
    return value;
}

bool AssetDB::kv_has(const uint64 key, const HashType type) const {
    SQLite::Statement s(m_db, "SELECT 1 FROM kv WHERE lookup3=?1 OR (murmur<>0 AND murmur=?1) LIMIT 1");
    s.bind(1, sql_hash(key));
    return s.executeStep();
}

void AssetDB::kv_del(const uint64 key, const HashType type) const {
    SQLite::Statement s(m_db, "DELETE FROM kv WHERE lookup3=?1 OR (murmur<>0 AND murmur=?1)");
    s.bind(1, sql_hash(key));
    s.exec();
}

void AssetDB::files_put(StringHashes hashes, std::string_view name, const uint64 size,
                        const uint64 parent_hash) const {
    auto &existing = *m_files_pair;
    existing.reset();
    existing.bind(1, static_cast<int64>(hashes.lookup3));
    existing.bind(2, sql_hash(hashes.murmur));
    std::string existing_name;
    if (existing.executeStep()) {
        const auto &column = existing.getColumn(0);
        const std::string_view stored(column.getText(), column.getBytes());
        if (!stored.empty() && !name.empty() && stored != name)
            throw std::runtime_error(std::format("Different file names have identical Lookup3 and Murmur hashes: \"{}\" - \"{}\"", stored, name));
        if (name.empty()) existing_name.assign(stored);
    }
    existing.reset();
    if (name.empty()) name = existing_name;

    auto &s = *m_files_put;
    s.reset();
    s.bind(1, static_cast<int64>(hashes.lookup3));
    s.bind(2, sql_hash(hashes.murmur));
    s.bind(3, std::string(name));
    s.bind(4, sql_hash(size));
    s.bind(5, sql_hash(parent_hash));
    s.exec();
}

std::optional<AssetDB::File> AssetDB::get_file(const uint64 hash, const HashType type) const {
    auto &s = *m_files_get;
    s.reset();
    s.bind(1, sql_hash(hash));
    if (!s.executeStep()) return std::nullopt;
    File result{
        static_cast<uint32>(s.getColumn(0).getInt64()), static_cast<uint64>(s.getColumn(1).getInt64()),
        s.getColumn(2).isNull() ? "" : s.getColumn(2).getString(),
        static_cast<uint64>(s.getColumn(3).getInt64()), static_cast<uint64>(s.getColumn(4).getInt64())
    };
    if (s.executeStep()) return std::nullopt;
    return result;
}

std::optional<std::string> AssetDB::get_file_name(const uint64 hash, const HashType type) const {
    auto &s = *m_files_name;
    s.reset();
    s.bind(1, sql_hash(hash));
    if (!s.executeStep()) return std::nullopt;
    auto name = s.getColumn(0).isNull() ? "" : s.getColumn(0).getString();
    if (s.executeStep() || name.empty()) return std::nullopt;
    return name;
}

std::optional<uint64> AssetDB::get_file_size(const uint64 hash, const HashType type) const {
    if (const auto f = get_file(hash, type)) return f->size;
    return std::nullopt;
}

std::optional<uint64> AssetDB::get_file_parent(const uint64 hash, const HashType type) const {
    if (const auto f = get_file(hash, type)) return f->parent_hash;
    return std::nullopt;
}

void AssetDB::files_del(const uint64 hash, const HashType type) const {
    auto &s = *m_files_del;
    s.reset();
    s.bind(1, sql_hash(hash));
    s.exec();
}

void AssetDB::files_search(const std::string_view pattern, std::vector<std::string> &out) const {
    auto &s = *m_files_search;
    s.reset();
    s.bind(1, std::string(pattern));
    out.clear();
    while (s.executeStep()) if (!s.getColumn(0).isNull()) out.push_back(s.getColumn(0).getString());
}

AssetDB::AssetDB(const std::filesystem::path &path) : AssetDB(path, false) {
}

AssetDB::AssetDB(const std::filesystem::path &path, const bool create)
    : m_db(path.string(), create ? SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE : SQLite::OPEN_READWRITE),
      m_files_table(game_files_table()) {
    if (!m_files_table.starts_with("files_") ||
        !std::all_of(m_files_table.begin(), m_files_table.end(),
                     [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'; }))
        throw std::invalid_argument("Invalid game files table name");
    if (create) init_new();
    else migrate();
    m_db.exec("PRAGMA journal_mode=WAL;");
    m_db.exec("PRAGMA synchronous=NORMAL;");
    m_db.exec("PRAGMA temp_store=MEMORY;");
    m_db.exec("PRAGMA wal_autocheckpoint=10000;");
    m_kv_put = std::make_unique<SQLite::Statement>(m_db, "INSERT INTO kv(lookup3,murmur,v) VALUES(?,?,?)");
    m_kv_pair = std::make_unique<SQLite::Statement>(m_db, "SELECT v FROM kv WHERE lookup3=?1 AND murmur=?2");
    m_files_pair = std::make_unique<SQLite::Statement>(
        m_db, std::format("SELECT name FROM {} WHERE lookup3=?1 AND murmur=?2", m_files_table));
    m_files_put = std::make_unique<SQLite::Statement>(
        m_db, std::format("INSERT OR REPLACE INTO {}(lookup3,murmur,name,size,parent) VALUES(?,?,?,?,?)",
                          m_files_table));
    m_files_search = std::make_unique<SQLite::Statement>(
        m_db, std::format("SELECT name FROM {} WHERE name LIKE ? ESCAPE '\\'", m_files_table));
    m_files_get = std::make_unique<SQLite::Statement>(m_db, std::format(
        "SELECT lookup3,murmur,name,size,parent FROM {} WHERE lookup3=?1 OR (murmur<>0 AND murmur=?1) LIMIT 2",
        m_files_table));
    m_files_name = std::make_unique<SQLite::Statement>(m_db, std::format(
        "SELECT name FROM {} WHERE lookup3=?1 OR (murmur<>0 AND murmur=?1) LIMIT 2", m_files_table));
    m_files_del = std::make_unique<SQLite::Statement>(m_db, std::format(
        "DELETE FROM {} WHERE lookup3=?1 OR (murmur<>0 AND murmur=?1)", m_files_table));
}

AssetDB AssetDB::create_new(const std::filesystem::path &path) { return AssetDB{path, true}; }
