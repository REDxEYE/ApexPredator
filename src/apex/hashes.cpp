// Created by RED on 07.10.2025.

#include "apex/hashes.h"

#include <cstring>

#include "platform/logger.h"
#include "utils/sqlite_wrapper.h"

static assetdb_t *assets_db;
static std::filesystem::path db_path = "./hashes.db";

void set_db_path(const std::filesystem::path &path) {
    db_path = path;
}

const std::filesystem::path& get_db_path() {
    return db_path;
}

void init_assets_db() {
    if (assets_db == nullptr) {
        if (assetdb_open(&assets_db, db_path) != KV_OK) {
            GLog_Error("Failed to open hashes database");
            abort();
        }
    }
}

void close_assets_db() {
    if (assets_db != nullptr) {
        assetdb_close(assets_db);
        assets_db = nullptr;
    }
}

assetdb_t *get_assets_db() {
    if (assets_db == nullptr) {
        init_assets_db();
    }
    return assets_db;
}

std::optional<std::string_view> find_name32_sv(const uint32 key) {
    return find_name64_sv(key);
}

std::optional<std::string_view> find_name64_sv(const uint64 key) {
    const char *value = nullptr;
    size_t value_len;
    const assetdb_status_t status = assetdb_kv_get_u64_view(get_assets_db(), key, &value, &value_len);
    if (status == KV_NOTFOUND || value == nullptr) {
        return std::nullopt;
    }
    return std::string_view(value, value_len);
}

std::optional<std::string> find_name32(const uint32 key) {
    return find_name64(key);
}

std::optional<std::string> find_name64(const uint64 key) {
    const char *value = nullptr;
    size_t value_len;
    const assetdb_status_t status = assetdb_kv_get_u64_view(get_assets_db(), key, &value, &value_len);
    if (status == KV_NOTFOUND) {
        return std::nullopt;
    }
    return std::string(value, value_len);
}

bool check_hash32_presence(const uint32 key) {
    return check_hash64_presence(key);
}

bool check_hash64_presence(const uint64 key) {
    const char *stored_value = nullptr;
    const assetdb_status_t status = assetdb_kv_get_u64_view(get_assets_db(), key, &stored_value, nullptr);
    if (status == KV_NOTFOUND) {
        return false;
    }
    if (!stored_value || strlen(stored_value) == 0) {
        return false;
    }
    return true;
}

void store_hash32_name(const uint32 key, const std::string_view &value) {
    store_hash64_name(key,value);
}

void store_hash64_name(const uint64 key, const std::string_view &value) {
    if (value.empty())return;
    assetdb_kv_put_u64(get_assets_db(), key, value.data());
}

void search_file_table(const std::string_view pattern, std::vector<std::string> &result) {
    const assetdb_status_t status = assetdb_files_search(get_assets_db(), pattern, result);
    if (status != KV_OK) {
        GLog_Error("Failed to search file table with pattern '{}': {}", pattern, std::to_string(status));
    }
}

std::optional<uint32> get_file_parent(uint64 key) {
    uint64 parent_hash;
    const assetdb_status_t status = assetdb_files_get_view(get_assets_db(), key, nullptr, nullptr, nullptr, &parent_hash);
    if (status != KV_OK) {
        return std::nullopt;
    }
    return static_cast<uint32>(parent_hash);
}

std::optional<std::string> get_file_parent(const uint64 key, uint64& out_parent) {
    const char *name;
    size_t name_len;
    const assetdb_status_t status = assetdb_files_get_view(get_assets_db(), key, &name, &name_len, nullptr, &out_parent);
    if (status != KV_OK) {
        return std::nullopt;
    }
    if (name == nullptr) {
        return std::nullopt;
    }
    return std::string(name, name_len);
}

std::filesystem::path get_export_path(const std::filesystem::path &base_export_path, const uint32 hash, const std::string_view ext) {
    std::filesystem::path result = base_export_path;
    if (const std::optional<std::string> file_name = find_name32(hash)) {
        result /= *file_name;
    }
    else {
        char tmp[64];
        std::snprintf(tmp, sizeof(tmp), "file_%08X%s", hash, ext.data());
        result /= tmp;
    }

    return result;
}
