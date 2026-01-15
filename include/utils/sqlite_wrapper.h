// Created by RED on 15.01.2026.

#ifndef APEXPREDATOR_SQLITE_WRAPPER_H
#define APEXPREDATOR_SQLITE_WRAPPER_H
#include <stdint.h>

typedef struct kvdb kvdb_t;

typedef enum kv_status {
    KV_OK = 0,
    KV_NOTFOUND = 1,
    KV_EINVAL = -1,
    KV_ESQLITE = -2,
    KV_ENOMEM = -3
} kv_status_t;

kv_status_t kv_open(kvdb_t **out_db, const char *path);
void        kv_close(kvdb_t *db);

kv_status_t kv_put_u64(kvdb_t *db, uint64_t key, const char *value);
kv_status_t kv_get_u64(kvdb_t *db, uint64_t key, char **out_value); /* caller frees */
kv_status_t kv_del_u64(kvdb_t *db, uint64_t key);

kv_status_t kv_put_u32(kvdb_t *db, uint32_t key, const char *value);
kv_status_t kv_get_u32(kvdb_t *db, uint32_t key, char **out_value); /* caller frees */
kv_status_t kv_del_u32(kvdb_t *db, uint32_t key);

const char *kv_last_error(const kvdb_t *db);
int         kv_last_sqlite_code(const kvdb_t *db);


#endif //APEXPREDATOR_SQLITE_WRAPPER_H