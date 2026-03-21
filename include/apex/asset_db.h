// Created by RED on 15.01.2026.

#ifndef APEXPREDATOR_SQLITE_WRAPPER_H
#define APEXPREDATOR_SQLITE_WRAPPER_H
#include <filesystem>
#include <string>
#include <vector>

#include "int_def.h"
#include "SQLiteCpp/SQLiteCpp.h"

class AssetDB {
public:
    struct File;

    static void set_instance(AssetDB *db);
    static AssetDB *get_instance();
    explicit AssetDB(const std::filesystem::path &path);


    void init_new();

    bool kv_has(uint64_t key);
    void kv_put(uint64_t key, const char *value);
    std::optional<std::string>  kv_get(uint64_t key);
    void kv_del(uint64_t key);

    void files_put(uint64_t hash, std::string_view name, uint64 size, uint64 parent_hash);
    std::optional<std::string>  get_file_name(uint64 hash);
    std::optional<uint64> get_file_size(uint64 hash);
    std::optional<uint64> get_file_parent(uint64 hash);
    std::optional<File> get_file(uint64 hash);
    void files_del(uint64 hash);
    void files_search(std::string_view pattern, std::vector<std::string> &out);

    struct File {
        uint64_t hash;
        std::string name;
        uint64_t size;
        uint64_t parent_hash;
    };

private:
    SQLite::Column exec_sql(const char *sql);


    SQLite::Database m_db;
    static std::filesystem::path db_path;

    SQLite::Statement m_kv_put;
    SQLite::Statement m_kv_get;
    SQLite::Statement m_kv_del;

    SQLite::Statement m_files_search;
    SQLite::Statement m_files_put;
    SQLite::Statement m_files_get;
    SQLite::Statement m_files_del;
};

#endif //APEXPREDATOR_SQLITE_WRAPPER_H
