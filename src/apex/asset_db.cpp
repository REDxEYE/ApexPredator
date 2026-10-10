#include "apex/asset_db.h"
#include "games.hpp"
#include <bit>

#include "redscore/platform/logger.h"

namespace {
    AssetDB *instance = nullptr;
    int64 sql_hash(const uint64 hash) { return std::bit_cast<int64>(hash); }


    constexpr auto schema =
            "CREATE TABLE kv (lookup3 INTEGER NOT NULL, murmur INTEGER NOT NULL DEFAULT 0,"
            "v TEXT NOT NULL, PRIMARY KEY(lookup3,murmur)) WITHOUT ROWID;"
            "CREATE INDEX idx_kv_murmur ON kv(murmur) WHERE murmur<>0;"
            "CREATE TABLE files (lookup3 INTEGER NOT NULL, murmur INTEGER NOT NULL DEFAULT 0,"
            "name TEXT, size INTEGER NOT NULL, parent INTEGER NOT NULL DEFAULT 0,"
            "PRIMARY KEY(lookup3,murmur)) WITHOUT ROWID;"
            "CREATE INDEX idx_files_murmur ON files(murmur) WHERE murmur<>0;"
            "CREATE INDEX idx_files_parent ON files(parent);";
}

void AssetDB::set_instance(AssetDB *db) { instance = db; }

AssetDB *AssetDB::get_instance() {
    if (!instance) throw std::runtime_error("AssetDB: instance is not set");
    return instance;
}

void AssetDB::init_new() {
    m_db.exec(schema);
}

void AssetDB::migrate() {
    bool legacy = false;
    {
        SQLite::Statement columns(m_db, "PRAGMA table_info(kv)");
        while (columns.executeStep())
            if (columns.getColumn(1).getString() == "k") legacy = true;
    }
    if (!legacy) return;
    SQLite::Transaction transaction(m_db);
    m_db.exec("ALTER TABLE kv RENAME TO kv_legacy; ALTER TABLE files RENAME TO files_legacy;"
        "DROP INDEX IF EXISTS idx_files_parent;");
    init_new();
    auto hashes_for = [](const uint64 key, const std::string &name) {
        auto hashes = string_hashes(name);
#if GAME==GAME_GENERATION_ZERO || GAME==GAME_JUST_CAUSE_2
        hashes.lookup3 = static_cast<uint32>(key);
#else
        hashes.murmur = key;
#endif
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
        SQLite::Statement insert(m_db, "INSERT INTO files VALUES(?,?,?,?,?)");
        while (rows.executeStep()) {
            auto hashes = hashes_for(static_cast<uint64>(rows.getColumn(0).getInt64()),
                                     rows.getColumn(1).isNull() ? "" : rows.getColumn(1).getString());
#if GAME==GAME_RAGE2 || GAME==GAME_SECOND_EXTINCTION
            if (rows.getColumn(1).isNull()) hashes.lookup3 = 0;
#endif
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
    auto &s = *m_kv_put;
    s.reset();
    s.bind(1, static_cast<int64>(hashes.lookup3));
    s.bind(2, sql_hash(hashes.murmur));
    s.bind(3, std::string(value));
    s.exec();
}

std::optional<std::string> AssetDB::kv_get(const uint64 key, const HashType type) const {
    SQLite::Statement s(m_db, "SELECT v FROM kv WHERE lookup3=?1 OR (murmur<>0 AND murmur=?1) ORDER BY lookup3,murmur LIMIT 1");
    s.bind(1, sql_hash(key));
    if (s.executeStep()) return s.getColumn(0).getString();
    return std::nullopt;
}

bool AssetDB::kv_has(const uint64 key, const HashType type) const { return kv_get(key, type).has_value(); }

void AssetDB::kv_del(const uint64 key, const HashType type) const {
    SQLite::Statement s(m_db, "DELETE FROM kv WHERE lookup3=?1 OR (murmur<>0 AND murmur=?1)");
    s.bind(1, sql_hash(key));
    s.exec();
}

void AssetDB::files_put(StringHashes hashes, std::string_view name, const uint64 size,
                        const uint64 parent_hash) const {
    auto existing = get_file(hashes.murmur != 0 ? hashes.murmur : hashes.lookup3,
                             hashes.murmur != 0 ? HashType::Murmur : HashType::Lookup3);
    if (!existing.has_value() && hashes.murmur != 0 && hashes.lookup3 != 0) {
        existing = get_file(hashes.lookup3, HashType::Lookup3);
    }

    if (existing.has_value()) {
        if (existing->murmur != 0 && hashes.murmur != 0 && existing->murmur != hashes.murmur) {
            GLog_Error("File with different murmur hash already exists");
            throw std::runtime_error("File with different murmur hash already exists");
        }
        if (existing->lookup3 != 0 && hashes.lookup3 != 0 && existing->lookup3 != hashes.lookup3) {
            GLog_Error("File with different lookup3 hash already exists");
            throw std::runtime_error("File with different lookup3 hash already exists");
        }
        if (!existing->name.empty() && !name.empty() && existing->name != name) {
            GLog_Error("File with different name already exists");
            throw std::runtime_error("File with different name already exists");
        }

        // if (existing->parent_hash != parent_hash) {
        //     auto cur_parent_path = get_file_name(existing->parent_hash).value_or(existing->parent_hash!=0?
        //         std::format("{}", existing->parent_hash):"<root archives>");
        //     auto new_parent_path = parent_hash != 0
        //                                ? get_file_name(parent_hash).value_or(std::format("{}", parent_hash))
        //                                : "<root archives>";
        //
        //     GLog_Error("File with different parent hash already exists \"{}\": {} -> {}",
        //                existing->name.empty()?name:existing->name, cur_parent_path, new_parent_path);
        //     // throw std::runtime_error("File with different parent hash already exists");
        // }

        if (!existing->name.empty() && name.empty()) {
            name = existing->name;
        }
        if (hashes.lookup3 == 0 && existing->lookup3 != 0) {
            hashes.lookup3 = existing->lookup3;
        }
        if (hashes.murmur == 0 && existing->murmur != 0) {
            hashes.murmur = existing->murmur;
        }
    }

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
    SQLite::Statement s(
        m_db, "SELECT lookup3,murmur,name,size,parent FROM files WHERE lookup3=?1 OR (murmur<>0 AND murmur=?1) ORDER BY lookup3,murmur LIMIT 1");
    s.bind(1, sql_hash(hash));
    if (!s.executeStep()) return std::nullopt;
    return File{
        static_cast<uint32>(s.getColumn(0).getInt64()), static_cast<uint64>(s.getColumn(1).getInt64()),
        s.getColumn(2).isNull() ? "" : s.getColumn(2).getString(),
        static_cast<uint64>(s.getColumn(3).getInt64()), static_cast<uint64>(s.getColumn(4).getInt64())
    };
}

std::optional<std::string> AssetDB::get_file_name(const uint64 hash, const HashType type) const {
    SQLite::Statement s(m_db, "SELECT name FROM files WHERE lookup3=?1 OR (murmur<>0 AND murmur=?1) ORDER BY lookup3,murmur LIMIT 1");
    s.bind(1, sql_hash(hash));
    if (s.executeStep() && !s.getColumn(0).isNull()) {
        auto name = s.getColumn(0).getString();
        if (name.empty()) return std::nullopt;
        return name;
    }
    return std::nullopt;
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
    SQLite::Statement s(m_db, "DELETE FROM files WHERE lookup3=?1 OR (murmur<>0 AND murmur=?1)");
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
    : m_db(path.string(), create ? SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE : SQLite::OPEN_READWRITE) {
    if (create) init_new();
    else migrate();
    m_db.exec("PRAGMA journal_mode=WAL;");
    m_db.exec("PRAGMA synchronous=NORMAL;");
    m_db.exec("PRAGMA temp_store=MEMORY;");
    m_db.exec("PRAGMA wal_autocheckpoint=10000;");
    m_kv_put = std::make_unique<SQLite::Statement>(m_db, "INSERT OR REPLACE INTO kv(lookup3,murmur,v) VALUES(?,?,?)");
    m_files_put = std::make_unique<SQLite::Statement>(
        m_db, "INSERT OR REPLACE INTO files(lookup3,murmur,name,size,parent) VALUES(?,?,?,?,?)");
    m_files_search = std::make_unique<SQLite::Statement>(m_db, "SELECT name FROM files WHERE name LIKE ? ESCAPE '\\'");
}

AssetDB AssetDB::create_new(const std::filesystem::path &path) { return AssetDB{path, true}; }
