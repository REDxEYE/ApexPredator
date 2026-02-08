// Created by RED on 15.01.2026.

#ifndef APEXPREDATOR_SQLITE_WRAPPER_H
#define APEXPREDATOR_SQLITE_WRAPPER_H
#include <stddef.h>
#include <stdint.h>

typedef struct assetdb assetdb_t;

typedef enum kv_status {
    KV_OK = 0,
    KV_NOTFOUND = 1,
    KV_EINVAL = -1,
    KV_ESQLITE = -2,
    KV_ENOMEM = -3
} assetdb_status_t;

assetdb_status_t assetdb_open(assetdb_t **out_db, const char *path);
void        assetdb_close(assetdb_t *db);

assetdb_status_t assetdb_kv_put_u64(assetdb_t *db, uint64_t key, const char *value);

assetdb_status_t assetdb_kv_get_u64_view(assetdb_t *db, uint64_t key, const char **out, size_t *out_len); /* db owned data */
assetdb_status_t assetdb_kv_del_u64(assetdb_t *db, uint64_t key);

assetdb_status_t assetdb_kv_put_u32(assetdb_t *db, uint32_t key, const char *value);

assetdb_status_t assetdb_kv_get_u32_view(assetdb_t *db, uint32_t key, const char **out, size_t *out_len); /* db owned data */
assetdb_status_t assetdb_kv_del_u32(assetdb_t *db, uint32_t key);


assetdb_status_t kv_vp_put_u64(assetdb_t *db, uint64_t child, uint64_t parent, const char *path);
assetdb_status_t kv_vp_get_u64(assetdb_t *db, uint64_t child, uint64_t *out_parent, const char **out_path, size_t *out_path_len); /* db owned data */
assetdb_status_t kv_vp_del_u64(assetdb_t *db, uint64_t child);

const char *assetdb_last_error(const assetdb_t *db);
int         assetdb_last_sqlite_code(const assetdb_t *db);


#endif //APEXPREDATOR_SQLITE_WRAPPER_H