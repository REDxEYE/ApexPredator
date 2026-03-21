#include "apex/asset_db.h"

#include <filesystem>
#include <vector>

#include "platform/logger.h"
#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Statement.h"

using namespace std::string_view_literals;

static AssetDB *instance = nullptr;

void AssetDB::set_instance(AssetDB *db) {
    instance = db;
}

AssetDB *AssetDB::get_instance() {
    if (instance == nullptr) {
       GLog_Error("AssetDB: instance is not set");
       throw std::runtime_error("AssetDB: instance is not set");
    }
    return instance;
}


void AssetDB::init_new() {
    if (m_db.tableExists("kv")) {
        m_db.exec("DROP TABLE kv;");
    }
    if (m_db.tableExists("files")) {
        m_db.exec("DROP TABLE files;");
    }
    m_db.exec("PRAGMA foreign_keys=ON;");
    m_db.exec("CREATE TABLE IF NOT EXISTS kv ("
        "k INTEGER NOT NULL,"
        "v TEXT NOT NULL,"
        "PRIMARY KEY(k)"
        ") WITHOUT ROWID;"
    );
    m_db.exec("CREATE TABLE IF NOT EXISTS files ("
        "hash   INTEGER NOT NULL,"
        "name   TEXT,"
        "size   INTEGER NOT NULL,"
        "parent INTEGER NOT NULL DEFAULT 0,"
        "PRIMARY KEY(hash)"
        ") WITHOUT ROWID;"
        "CREATE INDEX IF NOT EXISTS idx_files_parent ON files(parent);");
}

bool AssetDB::kv_has(const uint64_t key) {
    m_kv_get.reset();
    m_kv_get.bind(1, static_cast<int64>(key));
    return m_kv_get.executeStep();
}

void AssetDB::kv_put(const uint64 key, const char *value) {
    m_kv_put.reset();
    m_kv_put.bind(1, static_cast<int64>(key));
    m_kv_put.bind(2, value);
    m_kv_put.exec();
}

void AssetDB::kv_del(const uint64 key) {
    m_kv_del.reset();
    m_kv_del.bind(1, static_cast<int64>(key));
    m_kv_del.exec();
}

std::optional<std::string> AssetDB::kv_get(const uint64 key) {
    m_kv_get.reset();
    m_kv_get.bind(1, static_cast<int64>(key));

    if (!m_kv_get.executeStep()) {
        return std::nullopt;
    }

    const auto column = m_kv_get.getColumn(0);
    if (column.isNull()) {
        return std::nullopt;
    }

    return column.getString();
}

void AssetDB::files_put(const uint64 hash, const std::string_view name, const uint64 size, const uint64 parent_hash) {
    m_files_put.reset();
    m_files_put.clearBindings();
    m_files_put.bind(1, static_cast<int64>(hash));
    m_files_put.bind(2, name.data(), static_cast<int>(name.size()));
    m_files_put.bind(3, static_cast<int64>(size));
    m_files_put.bind(4, static_cast<int64>(parent_hash));
    m_files_put.exec();
}

std::optional<std::string> AssetDB::get_file_name(const uint64 hash) {
    m_files_get.reset();
    m_files_get.clearBindings();
    m_files_get.bind(1, static_cast<int64>(hash));

    if (!m_files_get.executeStep()) {
        return {};
    }

    const auto column = m_files_get.getColumn(0);
    if (column.isNull()) {
        return {};
    }

    return column.getString();
}

std::optional<uint64> AssetDB::get_file_size(const uint64 hash) {
    m_files_get.reset();
    m_files_get.clearBindings();
    m_files_get.bind(1, static_cast<int64>(hash));

    if (!m_files_get.executeStep()) {
        return 0;
    }

    const auto column = m_files_get.getColumn(1);
    if (column.isNull()) {
        return 0;
    }

    return static_cast<uint64>(column.getInt64());
}

std::optional<uint64> AssetDB::get_file_parent(const uint64 hash) {
    m_files_get.reset();
    m_files_get.clearBindings();
    m_files_get.bind(1, static_cast<int64>(hash));

    if (!m_files_get.executeStep()) {
        return 0;
    }

    const auto column = m_files_get.getColumn(2);
    if (column.isNull()) {
        return 0;
    }

    return static_cast<uint64>(column.getInt64());
}

std::optional<AssetDB::File> AssetDB::get_file(const uint64 hash) {
    m_files_get.reset();
    m_files_get.clearBindings();
    m_files_get.bind(1, static_cast<int64>(hash));

    if (!m_files_get.executeStep()) {
        return {};
    }

    File file{};
    file.hash = hash;

    {
        const auto column = m_files_get.getColumn(0);
        if (!column.isNull()) {
            file.name = column.getString();
        }
    }

    {
        const auto column = m_files_get.getColumn(1);
        if (!column.isNull()) {
            file.size = static_cast<uint64>(column.getInt64());
        }
    }

    {
        const auto column = m_files_get.getColumn(2);
        if (!column.isNull()) {
            file.parent_hash = static_cast<uint64>(column.getInt64());
        }
    }

    return file;
}

void AssetDB::files_del(const uint64 hash) {
    m_files_del.reset();
    m_files_del.clearBindings();
    m_files_del.bind(1, static_cast<int64>(hash));
    m_files_del.exec();
}

void AssetDB::files_search(const std::string_view pattern, std::vector<std::string>& out) {
    auto escape_like = [](const std::string_view value) {
        std::string escaped;
        escaped.reserve(value.size() * 2 + 2);
        escaped.push_back('%');
        for (char c : value) {
            if (c == '%' || c == '_' || c == '\\') {
                escaped.push_back('\\');
            }
            escaped.push_back(c);
        }
        escaped.push_back('%');
        return escaped;
    };

    out.clear();

    const std::string escaped = escape_like(pattern);

    m_files_search.reset();
    m_files_search.clearBindings();
    m_files_search.bind(1, escaped.c_str());

    while (m_files_search.executeStep()) {
        const auto column = m_files_search.getColumn(0);
        if (!column.isNull()) {
            out.emplace_back(column.getString());
        }
    }
}

SQLite::Column AssetDB::exec_sql(const char *sql) {
    return m_db.execAndGet(sql);
}

AssetDB::AssetDB(const std::filesystem::path &path)
    : m_db(path, SQLite::OPEN_READWRITE),
      m_kv_put(m_db, "INSERT OR REPLACE INTO kv(k, v) VALUES(?, ?);"),
      m_kv_get(m_db, "SELECT v FROM kv WHERE k=?;"),
      m_kv_del(m_db, "DELETE FROM kv WHERE k=?;"),
      m_files_search(m_db, "SELECT name FROM files WHERE name LIKE ? ESCAPE '\\';"),
      m_files_put(m_db, "INSERT OR REPLACE INTO files(hash, name, size, parent) VALUES(?, ?, ?, ?)"),
      m_files_get(m_db, "SELECT name, size, parent FROM files WHERE hash=?;"),
      m_files_del(m_db, "DELETE FROM files WHERE hash=?;") {
}
