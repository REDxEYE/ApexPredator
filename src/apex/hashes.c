// Created by RED on 07.10.2025.

#include "apex/hashes.h"
#include "apex/adf/adf_types.h"
#include "platform/common_arrays.h"
#include "platform/logger.h"
#include "utils/sqlite_wrapper.h"

static kvdb_t *hash_db;

void init_hashes() {
    if (hash_db == NULL) {
        if (kv_open(&hash_db, "./../hashes.db") != KV_OK) {
            GLog_Error("Failed to open hashes database");
            exit(1);
        }
    }
}

void close_hash_db() {
    if (hash_db != NULL) {
        kv_close(hash_db);
        hash_db = NULL;
    }
}

kvdb_t *get_hash_db() {
    if (hash_db == NULL) {
        init_hashes();
    }
    return hash_db;
}

String *find_name32(const uint32 key) {
    init_hashes();
    const char *value = NULL;
    size_t value_len;
    const kv_status_t status = kv_get_u32_view(hash_db, key, &value, &value_len);
    if (status == KV_NOTFOUND || value==NULL) {
        return NULL;
    }
    String *tmp = String_new_from_cstr2(value, value_len);
    return tmp;
}

String * find_name64(const uint64 key) {
    init_hashes();
    const char *value = NULL;
    size_t value_len;
    const kv_status_t status = kv_get_u64_view(hash_db, key, &value, &value_len);
    if (status == KV_NOTFOUND || value==NULL) {
        return NULL;
    }
    String *tmp = String_new_from_cstr2(value, value_len);
    return tmp;
}

bool check_hash32_presence(const uint32 key) {
    init_hashes();
    char *stored_value = NULL;
    const kv_status_t status = kv_get_u32(hash_db, key, &stored_value);
    if (status == KV_NOTFOUND) {
        return false;
    }
    mp_free(stored_value);
    return true;
}

bool check_hash64_presence(const uint64 key) {
    init_hashes();
    char *stored_value = NULL;
    const kv_status_t status = kv_get_u64(hash_db, key, &stored_value);
    if (status == KV_NOTFOUND) {
        return false;
    }
    mp_free(stored_value);
    return true;
}

void store_hash32_name(const uint32 key, const String *value) {
    init_hashes();
    kv_put_u32(hash_db, key, String_cstr(value));
}

void store_hash64_name(const uint64 key, const String *value) {
    init_hashes();
    kv_put_u64(hash_db, key, String_cstr(value));
}
