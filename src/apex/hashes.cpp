// Created by RED on 07.10.2025.

#include "apex/hashes.h"

#include <cstring>

#include "redscore/platform/logger.h"
#include "apex/asset_db.h"

std::optional<std::string> find_name(const uint64 key) {
    static auto db = AssetDB::get_instance();
    return db->kv_get(key);
}

bool check_hash_presence(const uint64 key) {
    static auto db = AssetDB::get_instance();
    return db->kv_has(key);
}

void store_hash_name(const uint64 key, const std::string_view &value) {
    static auto db = AssetDB::get_instance();
    db->kv_put(key, value.data());
}

void search_file_table(const std::string_view pattern, std::vector<std::string> &result) {
    static auto db = AssetDB::get_instance();
    db->files_search(pattern, result);
}

std::optional<uint32> get_file_parent(const uint64 key) {
    static auto db = AssetDB::get_instance();
    return db->get_file_parent(key);
}

std::optional<std::string> get_file_parent(const uint64 key, uint64& out_parent) {
    static auto db = AssetDB::get_instance();
    if (const auto file = db->get_file(key)) {
        out_parent = file->parent_hash;
        return file->name;
    }
    return std::nullopt;
}

std::filesystem::path get_export_path(const std::filesystem::path &base_export_path, const uint32 hash, const std::string_view ext) {
    static auto db = AssetDB::get_instance();
    std::filesystem::path result = base_export_path;
    if (const auto file_name = db->get_file_name(hash)) {
        result /= file_name.value();
    }
    else {
        char tmp[64];
        std::snprintf(tmp, sizeof(tmp), "file_%08X%s", hash, ext.data());
        result /= tmp;
    }

    return result;
}
