// Created by RED on 07.10.2025.

#include "apex/hashes.h"

#include <inttypes.h>

#include "platform/common_arrays.h"
#include "platform/logger.h"
#include "utils/sqlite_wrapper.h"

static assetdb_t *assets_db;

void init_assets_db() {
    if (assets_db == NULL) {
        if (assetdb_open(&assets_db, "./../hashes.db") != KV_OK) {
            GLog_Error("Failed to open hashes database");
            abort();
        }
    }
}

void close_assets_db() {
    if (assets_db != NULL) {
        assetdb_close(assets_db);
        assets_db = NULL;
    }
}

assetdb_t *get_assets_db() {
    if (assets_db == NULL) {
        init_assets_db();
    }
    return assets_db;
}

StringView find_name32_sv(const uint32 key) {
    const char *value = NULL;
    size_t value_len;
    const assetdb_status_t status = assetdb_kv_get_u32_view(get_assets_db(), key, &value, &value_len);
    if (status == KV_NOTFOUND || value == NULL) {
        return StringView_empty();
    }
    return StringView_from_cstr2(value, value_len);
}

StringView find_name64_sv(const uint64 key) {
    const char *value = NULL;
    size_t value_len;
    const assetdb_status_t status = assetdb_kv_get_u64_view(get_assets_db(), key, &value, &value_len);
    if (status == KV_NOTFOUND || value == NULL) {
        return StringView_empty();
    }
    return StringView_from_cstr2(value, value_len);
}

String * find_name32(const uint32 key) {
    const char *value = NULL;
    size_t value_len;
    const assetdb_status_t status = assetdb_kv_get_u64_view(get_assets_db(), key, &value, &value_len);
    if (status == KV_NOTFOUND) {
        return NULL;
    }
    String *result = String_new_from_cstr2(value, value_len);
    return result;
}

String * find_name64(const uint64 key) {
    const char *value = NULL;
    size_t value_len;
    const assetdb_status_t status = assetdb_kv_get_u64_view(get_assets_db(), key, &value, &value_len);
    if (status == KV_NOTFOUND) {
        return NULL;
    }
    String *result = String_new_from_cstr2(value, value_len);
    return result;
}

bool check_hash32_presence(const uint32 key) {
    const char *stored_value = NULL;
    const assetdb_status_t status = assetdb_kv_get_u64_view(get_assets_db(), key, &stored_value, NULL);
    if (status == KV_NOTFOUND) {
        return false;
    }
    return true;
}

bool check_hash64_presence(const uint64 key) {
    const char *stored_value = NULL;
    const assetdb_status_t status = assetdb_kv_get_u64_view(get_assets_db(), key, &stored_value, NULL);
    if (status == KV_NOTFOUND) {
        return false;
    }
    return true;
}

void store_hash32_name(const uint32 key, const String *value) {
    if (String_size(value) == 0 || String_cstr(value)[0] == '\0')return;
    assetdb_kv_put_u32(get_assets_db(), key, String_cstr(value));
}

void store_hash64_name(const uint64 key, const String *value) {
    if (String_size(value) == 0 || String_cstr(value)[0] == '\0')return;
    assetdb_kv_put_u64(get_assets_db(), key, String_cstr(value));
}

void store_file_parent(const uint64 key, const String *path, const uint64 parent) {
    const assetdb_status_t status =
            kv_vp_put_u64(get_assets_db(), key, parent, path != NULL ? String_cstr(path) : NULL);
    if (status != KV_OK) {
        GLog_Error("Failed to store file parent for key %llu: %d", key, status);
    }
}
void store_file_parent_sv(const uint64 key, const StringView path, const uint64 parent) {
    const assetdb_status_t status =
            kv_vp_put_u64(get_assets_db(), key, parent, sv_is_not_null(path) ? StringView_cstr(path) : NULL);
    if (status != KV_OK) {
        GLog_Error("Failed to store file parent for key %llu: %d", key, status);
    }
}

bool get_file_parent(const uint64 key, uint64 *out_parent, String **out_path) {
    const char *path = NULL;
    if (out_path == NULL) {
        const assetdb_status_t status = kv_vp_get_u64(get_assets_db(), key, out_parent, NULL, NULL);
        if (status != KV_OK) {
            return false;
        }
    }
    else {
        *out_path = NULL;
        *out_parent = 0;
        size_t path_len = 0;
        const assetdb_status_t status = kv_vp_get_u64(get_assets_db(), key, out_parent, &path, &path_len);
        if (status != KV_OK) {
            return false;
        }
        if (path && path_len > 0) {
            *out_path = String_new_from_cstr2(path, path_len);
        }
    }

    return true;
}
