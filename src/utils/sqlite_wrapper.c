// Created by RED on 15.01.2026.

#include "utils/sqlite_wrapper.h"


#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

struct kvdb {
    sqlite3 *db;
    sqlite3_stmt *st_put;
    sqlite3_stmt *st_get;
    sqlite3_stmt *st_del;
    char *last_err;
    int last_sqlite;
};

static kv_status_t set_err(kvdb_t *db, int sqlite_rc, const char *msg) {
    if (!db) return KV_EINVAL;
    db->last_sqlite = sqlite_rc;
    if (db->last_err) {
        sqlite3_free(db->last_err);
        db->last_err = NULL;
    }
    if (msg) db->last_err = sqlite3_mprintf("%s", msg);
    return KV_ESQLITE;
}

static kv_status_t exec_sql(kvdb_t *db, const char *sql) {
    char *errmsg = NULL;
    const int rc = sqlite3_exec(db->db, sql, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        const kv_status_t st = set_err(db, rc, errmsg ? errmsg : "sqlite3_exec failed");
        if (errmsg) sqlite3_free(errmsg);
        return st;
    }
    return KV_OK;
}

static kv_status_t prepare_stmt(kvdb_t *db, sqlite3_stmt **out, const char *sql) {
    const int rc = sqlite3_prepare_v2(db->db, sql, -1, out, NULL);
    if (rc != SQLITE_OK) return set_err(db, rc, sqlite3_errmsg(db->db));
    return KV_OK;
}

static kv_status_t db_init(kvdb_t *db) {
    kv_status_t st = exec_sql(db,
                              "PRAGMA journal_mode=WAL;"
                              "PRAGMA synchronous=NORMAL;"
                              "PRAGMA foreign_keys=ON;"
    );
    if (st != KV_OK) return st;

    st = exec_sql(db,
        "CREATE TABLE IF NOT EXISTS kv ("
        "  k INTEGER PRIMARY KEY,"
        "  v TEXT NOT NULL"
        ");"
    );
    if (st != KV_OK) return st;

    st = prepare_stmt(db, &db->st_put,
        "INSERT INTO kv(k, v) VALUES(?1, ?2) "
        "ON CONFLICT(k) DO UPDATE SET v=excluded.v;"
    );
    if (st != KV_OK) return st;

    st = prepare_stmt(db, &db->st_get,
        "SELECT v FROM kv WHERE k=?1;"
    );
    if (st != KV_OK) return st;

    st = prepare_stmt(db, &db->st_del,
        "DELETE FROM kv WHERE k=?1;"
    );
    if (st != KV_OK) return st;

    return KV_OK;
}

kv_status_t kv_open(kvdb_t **out_db, const char *path) {
    if (!out_db || !path) return KV_EINVAL;
    *out_db = NULL;

    kvdb_t *db = (kvdb_t *)calloc(1, sizeof(kvdb_t));
    if (!db) return KV_ENOMEM;

    const int rc = sqlite3_open_v2(path, &db->db,
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                            NULL);
    if (rc != SQLITE_OK) {
        const kv_status_t st = set_err(db, rc, db->db ? sqlite3_errmsg(db->db) : "sqlite3_open_v2 failed");
        if (db->db) sqlite3_close(db->db);
        free(db);
        return st;
    }

    const kv_status_t st = db_init(db);
    if (st != KV_OK) {
        kv_close(db);
        return st;
    }

    *out_db = db;
    return KV_OK;
}

void kv_close(kvdb_t *db) {
    if (!db) return;

    if (db->st_put) sqlite3_finalize(db->st_put);
    if (db->st_get) sqlite3_finalize(db->st_get);
    if (db->st_del) sqlite3_finalize(db->st_del);

    if (db->last_err) sqlite3_free(db->last_err);

    if (db->db) sqlite3_close(db->db);
    free(db);
}

static kv_status_t bind_u64(kvdb_t *db, sqlite3_stmt *st, int idx, const uint64_t key) {
    if (key > (uint64_t)INT64_MAX) return KV_EINVAL;
    const int rc = sqlite3_bind_int64(st, idx, (sqlite3_int64)key);
    if (rc != SQLITE_OK) return set_err(db, rc, sqlite3_errmsg(db->db));
    return KV_OK;
}

kv_status_t kv_put_u64(kvdb_t *db, const uint64_t key, const char *value) {
    if (!db || !value) return KV_EINVAL;

    sqlite3_reset(db->st_put);
    sqlite3_clear_bindings(db->st_put);

    const kv_status_t st = bind_u64(db, db->st_put, 1, key);
    if (st != KV_OK) return st;

    int rc = sqlite3_bind_text(db->st_put, 2, value, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) return set_err(db, rc, sqlite3_errmsg(db->db));

    rc = sqlite3_step(db->st_put);
    if (rc != SQLITE_DONE) return set_err(db, rc, sqlite3_errmsg(db->db));

    return KV_OK;
}

kv_status_t kv_get_u64(kvdb_t *db, const uint64_t key, char **out_value) {
    if (!db || !out_value) return KV_EINVAL;
    *out_value = NULL;

    sqlite3_reset(db->st_get);
    sqlite3_clear_bindings(db->st_get);

    const kv_status_t st = bind_u64(db, db->st_get, 1, key);
    if (st != KV_OK) return st;

    const int rc = sqlite3_step(db->st_get);
    if (rc == SQLITE_ROW) {
        const unsigned char *txt = sqlite3_column_text(db->st_get, 0);
        const int n = sqlite3_column_bytes(db->st_get, 0);
        char *s = (char *)malloc((size_t)n + 1);
        if (!s) return KV_ENOMEM;
        memcpy(s, txt ? (const char *)txt : "", (size_t)n);
        s[n] = '\0';
        *out_value = s;
        return KV_OK;
    }
    if (rc == SQLITE_DONE) return KV_NOTFOUND;

    return set_err(db, rc, sqlite3_errmsg(db->db));
}

kv_status_t kv_del_u64(kvdb_t *db, const uint64_t key) {
    if (!db) return KV_EINVAL;

    sqlite3_reset(db->st_del);
    sqlite3_clear_bindings(db->st_del);

    const kv_status_t st = bind_u64(db, db->st_del, 1, key);
    if (st != KV_OK) return st;

    const int rc = sqlite3_step(db->st_del);
    if (rc != SQLITE_DONE) return set_err(db, rc, sqlite3_errmsg(db->db));

    const int changes = sqlite3_changes(db->db);
    return (changes > 0) ? KV_OK : KV_NOTFOUND;
}

kv_status_t kv_put_u32(kvdb_t *db, const uint32_t key, const char *value) {
    return kv_put_u64(db, (uint64_t)key, value);
}
kv_status_t kv_get_u32(kvdb_t *db, const uint32_t key, char **out_value) {
    return kv_get_u64(db, (uint64_t)key, out_value);
}
kv_status_t kv_del_u32(kvdb_t *db, const uint32_t key) {
    return kv_del_u64(db, (uint64_t)key);
}

const char *kv_last_error(const kvdb_t *db) {
    return (db && db->last_err) ? db->last_err : NULL;
}

int kv_last_sqlite_code(const kvdb_t *db) {
    return db ? db->last_sqlite : 0;
}
