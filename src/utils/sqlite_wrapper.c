// Created by RED on 15.01.2026.

#include "utils/sqlite_wrapper.h"

#include <sqlite3.h>
#include <string.h>

#include "utils/memory_profiling.h"

struct assetdb {
    sqlite3 *db;
    sqlite3_stmt *kv_put;
    sqlite3_stmt *kv_get;
    sqlite3_stmt *kv_del;

    sqlite3_stmt *vp_put;
    sqlite3_stmt *vp_get;
    sqlite3_stmt *vp_del;

    char *last_err;
    int last_sqlite;
};

static assetdb_status_t set_err(assetdb_t *db, const int sqlite_rc, const char *msg) {
    if (!db) return KV_EINVAL;
    db->last_sqlite = sqlite_rc;
    if (db->last_err) {
        sqlite3_free(db->last_err);
        db->last_err = NULL;
    }
    if (msg) db->last_err = sqlite3_mprintf("%s", msg);
    return KV_ESQLITE;
}

static assetdb_status_t exec_sql(assetdb_t *db, const char *sql) {
    char *errmsg = NULL;
    const int rc = sqlite3_exec(db->db, sql, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        const assetdb_status_t st = set_err(db, rc, errmsg ? errmsg : "sqlite3_exec failed");
        if (errmsg) sqlite3_free(errmsg);
        return st;
    }
    return KV_OK;
}

static assetdb_status_t prepare_stmt(assetdb_t *db, sqlite3_stmt **out, const char *sql) {
    const int rc = sqlite3_prepare_v2(db->db, sql, -1, out, NULL);
    if (rc != SQLITE_OK) return set_err(db, rc, sqlite3_errmsg(db->db));
    return KV_OK;
}

static assetdb_status_t assetdb_init(assetdb_t *db) {
    assetdb_status_t st = exec_sql(db,
                             "PRAGMA journal_mode = WAL;"
                             "PRAGMA synchronous = NORMAL;"
                             "PRAGMA temp_store = MEMORY;"
                             "PRAGMA cache_size = -20000;"
                             "PRAGMA foreign_keys=ON;"
    );
    if (st != KV_OK) return st;

    st = exec_sql(db,
                  "CREATE TABLE IF NOT EXISTS kv ("
                  "k INTEGER NOT NULL,"
                  "v TEXT NOT NULL,"
                  "PRIMARY KEY(k)"
                  ") WITHOUT ROWID;"
    );
    if (st != KV_OK) return st;

    st = prepare_stmt(db, &db->kv_put,
                      "INSERT INTO kv(k, v) VALUES(?1, ?2) "
                      "ON CONFLICT(k) DO UPDATE SET v=excluded.v;"
    );
    if (st != KV_OK) return st;

    st = prepare_stmt(db, &db->kv_get,
                      "SELECT v FROM kv WHERE k=?1;"
    );
    if (st != KV_OK) return st;

    st = prepare_stmt(db, &db->kv_del,
                      "DELETE FROM kv WHERE k=?1;"
    );
    if (st != KV_OK) return st;

    st = exec_sql(db,
                  "CREATE TABLE IF NOT EXISTS vparent ("
                  "id INTEGER NOT NULL,"
                  "parent INTEGER NOT NULL DEFAULT 0,"
                  "path TEXT,"
                  "PRIMARY KEY(id)"
                  ") WITHOUT ROWID;"
                  "CREATE INDEX IF NOT EXISTS idx_vparent_parent ON vparent(parent);"
    );
    if (st != KV_OK) return st;

    st = prepare_stmt(db, &db->vp_put,
                      "INSERT INTO vparent(id, parent, path) VALUES(?1, ?2, ?3) "
                      "ON CONFLICT(id) DO UPDATE SET parent=excluded.parent, path=excluded.path;"
    );
    if (st != KV_OK) return st;

    st = prepare_stmt(db, &db->vp_get,
                      "SELECT parent, path FROM vparent WHERE id=?1;"
    );
    if (st != KV_OK) return st;

    st = prepare_stmt(db, &db->vp_del,
                      "DELETE FROM vparent WHERE id=?1;"
    );
    if (st != KV_OK) return st;

    return KV_OK;
}

assetdb_status_t assetdb_open(assetdb_t **out_db, const char *path) {
    if (!out_db || !path) return KV_EINVAL;
    *out_db = NULL;

    assetdb_t *db = (assetdb_t *) mp_calloc(1, sizeof(assetdb_t));
    if (!db) return KV_ENOMEM;

    const int rc = sqlite3_open_v2(path, &db->db,
                                   SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                                   NULL);
    if (rc != SQLITE_OK) {
        const assetdb_status_t st = set_err(db, rc, db->db ? sqlite3_errmsg(db->db) : "sqlite3_open_v2 failed");
        if (db->db) sqlite3_close(db->db);
        if (db->last_err) sqlite3_free(db->last_err);
        mp_free(db);
        return st;
    }

    const assetdb_status_t st = assetdb_init(db);
    if (st != KV_OK) {
        assetdb_close(db);
        return st;
    }

    *out_db = db;
    return KV_OK;
}

void assetdb_close(assetdb_t *db) {
    if (!db) return;

    if (db->kv_put) sqlite3_finalize(db->kv_put);
    if (db->kv_get) sqlite3_finalize(db->kv_get);
    if (db->kv_del) sqlite3_finalize(db->kv_del);

    if (db->vp_put) sqlite3_finalize(db->vp_put);
    if (db->vp_get) sqlite3_finalize(db->vp_get);
    if (db->vp_del) sqlite3_finalize(db->vp_del);

    if (db->last_err) sqlite3_free(db->last_err);

    if (db->db) sqlite3_close(db->db);
    mp_free(db);
}

static assetdb_status_t bind_u64(assetdb_t *db, sqlite3_stmt *st, const int idx, const uint64_t key) {
    if (key > (uint64_t) INT64_MAX) return KV_EINVAL;
    const int rc = sqlite3_bind_int64(st, idx, (sqlite3_int64) key);
    if (rc != SQLITE_OK) return set_err(db, rc, sqlite3_errmsg(db->db));
    return KV_OK;
}

assetdb_status_t assetdb_kv_put_u64(assetdb_t *db, const uint64_t key, const char *value) {
    if (!db || !value) return KV_EINVAL;

    sqlite3_reset(db->kv_put);
    sqlite3_clear_bindings(db->kv_put);

    const assetdb_status_t st = bind_u64(db, db->kv_put, 1, key);
    if (st != KV_OK) return st;

    int rc = sqlite3_bind_text(db->kv_put, 2, value, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) return set_err(db, rc, sqlite3_errmsg(db->db));

    rc = sqlite3_step(db->kv_put);
    if (rc != SQLITE_DONE) return set_err(db, rc, sqlite3_errmsg(db->db));

    return KV_OK;
}

assetdb_status_t assetdb_kv_get_u64_view(assetdb_t *db, const uint64_t key, const char **out, size_t *out_len) {
    if (!db || !out) return KV_EINVAL;
    *out = NULL;
    if (out_len) *out_len = 0;

    sqlite3_reset(db->kv_get);
    sqlite3_clear_bindings(db->kv_get);

    const assetdb_status_t st = bind_u64(db, db->kv_get, 1, key);
    if (st != KV_OK) return st;

    const int rc = sqlite3_step(db->kv_get);
    if (rc == SQLITE_ROW) {
        const unsigned char *txt = sqlite3_column_text(db->kv_get, 0);
        const int n = sqlite3_column_bytes(db->kv_get, 0);
        *out = (const char*)txt;
        if (out_len) *out_len = (size_t)n;
        return KV_OK;   // valid until next reset/step/finalize on st_get
    }
    if (rc == SQLITE_DONE) return KV_NOTFOUND;
    return set_err(db, rc, sqlite3_errmsg(db->db));
}

assetdb_status_t assetdb_kv_del_u64(assetdb_t *db, const uint64_t key) {
    if (!db) return KV_EINVAL;

    sqlite3_reset(db->kv_del);
    sqlite3_clear_bindings(db->kv_del);

    const assetdb_status_t st = bind_u64(db, db->kv_del, 1, key);
    if (st != KV_OK) return st;

    const int rc = sqlite3_step(db->kv_del);
    if (rc != SQLITE_DONE) return set_err(db, rc, sqlite3_errmsg(db->db));

    const int changes = sqlite3_changes(db->db);
    return (changes > 0) ? KV_OK : KV_NOTFOUND;
}

assetdb_status_t assetdb_kv_put_u32(assetdb_t *db, const uint32_t key, const char *value) {
    return assetdb_kv_put_u64(db, (uint64_t) key, value);
}

assetdb_status_t assetdb_kv_get_u32_view(assetdb_t *db, const uint32_t key, const char **out, size_t *out_len) {
    return assetdb_kv_get_u64_view(db, (uint64_t) key, out, out_len);
}

assetdb_status_t assetdb_kv_del_u32(assetdb_t *db, const uint32_t key) {
    return assetdb_kv_del_u64(db, (uint64_t) key);
}

assetdb_status_t kv_vp_put_u64(assetdb_t *db, const uint64_t child, const uint64_t parent, const char *path) {
    if (!db) return KV_EINVAL;

    sqlite3_reset(db->vp_put);
    sqlite3_clear_bindings(db->vp_put);

    assetdb_status_t st = bind_u64(db, db->vp_put, 1, child);
    if (st != KV_OK) return st;

    st = bind_u64(db, db->vp_put, 2, parent);
    if (st != KV_OK) return st;

    int rc;

    if (path) {
        rc = sqlite3_bind_text(db->vp_put, 3, path, -1, SQLITE_TRANSIENT);
    } else {
        rc = sqlite3_bind_null(db->vp_put, 3);
    }

    if (rc != SQLITE_OK) return set_err(db, rc, sqlite3_errmsg(db->db));

    const int step = sqlite3_step(db->vp_put);
    if (step != SQLITE_DONE) return set_err(db, step, sqlite3_errmsg(db->db));

    return KV_OK;
}

assetdb_status_t kv_vp_get_u64(assetdb_t *db, const uint64_t child, uint64_t *out_parent, const char **out_path,
                               size_t *out_path_len) {
    if (!db || !out_parent) return KV_EINVAL;
    *out_parent = 0;
    if (out_path) *out_path = NULL;
    if (out_path_len) *out_path_len = 0;

    sqlite3_reset(db->vp_get);
    sqlite3_clear_bindings(db->vp_get);

    assetdb_status_t st = bind_u64(db, db->vp_get, 1, child);
    if (st != KV_OK) return st;

    const int rc = sqlite3_step(db->vp_get);
    if (rc == SQLITE_ROW) {
        const sqlite3_int64 p = sqlite3_column_int64(db->vp_get, 0);
        if (p < 0) return KV_EINVAL;
        *out_parent = (uint64_t)p;

        if (out_path) {
            const unsigned char *txt = sqlite3_column_text(db->vp_get, 1);
            const int n = sqlite3_column_bytes(db->vp_get, 1);
            *out_path = (const char*)txt;
            if (out_path_len) *out_path_len = (size_t)(n > 0 ? n : 0);
        }
        return KV_OK;
    }
    if (rc == SQLITE_DONE) return KV_NOTFOUND;
    return set_err(db, rc, sqlite3_errmsg(db->db));
}

assetdb_status_t kv_vp_del_u64(assetdb_t *db, const uint64_t child) {
    if (!db) return KV_EINVAL;

    sqlite3_reset(db->vp_del);
    sqlite3_clear_bindings(db->vp_del);

    const assetdb_status_t st = bind_u64(db, db->vp_del, 1, child);
    if (st != KV_OK) return st;

    const int rc = sqlite3_step(db->vp_del);
    if (rc != SQLITE_DONE) return set_err(db, rc, sqlite3_errmsg(db->db));

    return (sqlite3_changes(db->db) > 0) ? KV_OK : KV_NOTFOUND;
}

assetdb_status_t kv_vp_search(assetdb_t *db, const char *pattern, char ***result, uint32* out_count) {
    if (!db || !pattern || !result) return KV_EINVAL;
    *result = NULL;

    char *sql = sqlite3_mprintf(
            "SELECT path FROM vparent WHERE path LIKE ?1 ESCAPE '\\';"
    );
    if (!sql) return KV_ENOMEM;

    sqlite3_stmt *st = NULL;
    assetdb_status_t stt = prepare_stmt(db, &st, sql);
    sqlite3_free(sql);
    if (stt != KV_OK) return stt;

    int rc = sqlite3_bind_text(st, 1, pattern, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(st);
        return set_err(db, rc, sqlite3_errmsg(db->db));
    }

    size_t capacity = 8;
    size_t count = 0;
    const char **res = (const char **) mp_malloc(capacity * sizeof(char*));
    if (!res) {
        sqlite3_finalize(st);
        return KV_ENOMEM;
    }

    while ((rc = sqlite3_step(st)) == SQLITE_ROW) {
        const unsigned char *txt = sqlite3_column_text(st, 0);
        if (txt) {
            if (count >= capacity) {
                capacity *= 2;
                const char **new_res = (const char **) mp_realloc(res, capacity * sizeof(char*));
                if (!new_res) {
                    mp_free(res);
                    sqlite3_finalize(st);
                    return KV_ENOMEM;
                }
                res = new_res;
            }
            const size_t str_size = strlen((const char*)txt) + 1;
            res[count] = mp_malloc(str_size);
            memcpy((char*)res[count], (const char*)txt, str_size);
            count++;
        }
    }
    sqlite3_finalize(st);

    if (rc != SQLITE_DONE) {
        mp_free(res);
        return set_err(db, rc, sqlite3_errmsg(db->db));
    }

    // NULL-terminate the result array
    if (count >= capacity) {
        const char **new_res = (const char **) mp_realloc(res, (capacity + 1) * sizeof(char*));
        if (!new_res) {
            mp_free(res);
            return KV_ENOMEM;
        }
        res = new_res;
    }
    res[count] = NULL;

    *result = res;
    *out_count = count;
    return KV_OK;
}

const char *assetdb_last_error(const assetdb_t *db) {
    return (db && db->last_err) ? db->last_err : NULL;
}

int assetdb_last_sqlite_code(const assetdb_t *db) {
    return db ? db->last_sqlite : 0;
}
