// Created by RED on 15.01.2026.

#ifndef APEXPREDATOR_SQLITE_WRAPPER_H
#define APEXPREDATOR_SQLITE_WRAPPER_H
#include <filesystem>
#include <string>
#include <vector>
#include <optional>
#include "utils/hash_helper.h"

#include "int_def.h"
#include "SQLiteCpp/SQLiteCpp.h"

class AssetDB {
public:
    struct File;
    enum class HashType { Game, Lookup3, Murmur };

    static void set_instance(AssetDB *db);
    static AssetDB *get_instance();
    explicit AssetDB(const std::filesystem::path &path);

    static AssetDB create_new(const std::filesystem::path &path);
    void init_new();

    bool kv_has(uint64_t key, HashType type = HashType::Game) const;
    void kv_put(StringHashes hashes, std::string_view value) const;
    std::optional<std::string>  kv_get(uint64_t key, HashType type = HashType::Game) const;
    void kv_del(uint64_t key, HashType type = HashType::Game) const;

    void files_put(StringHashes hashes, std::string_view name, uint64 size, uint64 parent_hash) const;
    std::optional<std::string>  get_file_name(uint64 hash, HashType type = HashType::Game) const;
    std::optional<uint64> get_file_size(uint64 hash, HashType type = HashType::Game) const;
    std::optional<uint64> get_file_parent(uint64 hash, HashType type = HashType::Game) const;
    std::optional<File> get_file(uint64 hash, HashType type = HashType::Game) const;
    void files_del(uint64 hash, HashType type = HashType::Game) const;
    void files_search(std::string_view pattern, std::vector<std::string> &out) const;

    struct File {
        uint32 lookup3;
        uint64 murmur;
        std::string name;
        uint64_t size;
        uint64_t parent_hash;
        uint64_t parent_murmur_hash;
    };

private:
    AssetDB(const std::filesystem::path &path, bool create_new);

    void migrate();


    SQLite::Database m_db;
    static std::filesystem::path db_path;

    std::unique_ptr<SQLite::Statement> m_kv_put;
    std::unique_ptr<SQLite::Statement> m_files_search;
    std::unique_ptr<SQLite::Statement> m_files_put;
};

#endif //APEXPREDATOR_SQLITE_WRAPPER_H
