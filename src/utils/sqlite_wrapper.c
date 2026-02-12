#include "utils/sqlite_wrapper.h"

#include <sqlite3.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "utils/memory_profiling.h"

#ifndef KV_NOTFOUND
#define KV_NOTFOUND 2
#endif

typedef struct assetdb assetdb_t;

struct assetdb {
    sqlite3 *db;

    sqlite3_stmt *kv_put;
    sqlite3_stmt *kv_get;
    sqlite3_stmt *kv_del;

    sqlite3_stmt *files_search;

    sqlite3_stmt *files_put;
    sqlite3_stmt *files_get;
    sqlite3_stmt *files_del;

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

typedef struct StmtSpec {
    sqlite3_stmt **dst;
    const char *sql;
} StmtSpec;

static assetdb_status_t prepare_many(assetdb_t *db, const StmtSpec *specs, const size_t n) {
    for (size_t i = 0; i < n; i++) {
        assetdb_status_t st = prepare_stmt(db, specs[i].dst, specs[i].sql);
        if (st != KV_OK) return st;
    }
    return KV_OK;
}

static inline void stmt_reset(sqlite3_stmt *st) {
    sqlite3_reset(st);
    sqlite3_clear_bindings(st);
}

static assetdb_status_t bind_u64_i64(assetdb_t *db, sqlite3_stmt *st, const int idx, const uint64_t v) {
    if (v > (uint64_t) INT64_MAX) return KV_EINVAL;
    const int rc = sqlite3_bind_int64(st, idx, (sqlite3_int64) v);
    if (rc != SQLITE_OK) return set_err(db, rc, sqlite3_errmsg(db->db));
    return KV_OK;
}

static assetdb_status_t bind_u32(assetdb_t *db, sqlite3_stmt *st, const int idx, const uint32_t v) {
    const int rc = sqlite3_bind_int(st, idx, (int) v);
    if (rc != SQLITE_OK) return set_err(db, rc, sqlite3_errmsg(db->db));
    return KV_OK;
}

static assetdb_status_t bind_text_nullable(assetdb_t *db, sqlite3_stmt *st, const int idx, const char *s) {
    const int rc = s
                       ? sqlite3_bind_text(st, idx, s, -1, SQLITE_TRANSIENT)
                       : sqlite3_bind_null(st, idx);
    if (rc != SQLITE_OK) return set_err(db, rc, sqlite3_errmsg(db->db));
    return KV_OK;
}

static assetdb_status_t stmt_step_done(assetdb_t *db, sqlite3_stmt *st) {
    const int rc = sqlite3_step(st);
    if (rc == SQLITE_DONE) return KV_OK;
    return set_err(db, rc, sqlite3_errmsg(db->db));
}

static void stmt_finalize(sqlite3_stmt **pst) {
    if (pst && *pst) {
        sqlite3_finalize(*pst);
        *pst = NULL;
    }
}

static const char *path_basename(const char *path) {
    if (!path || !*path) return NULL;
    const char *last = path;
    for (const char *p = path; *p; p++) {
        if (*p == '/' || *p == '\\') last = p + 1;
    }
    return (*last) ? last : NULL;
}

static assetdb_status_t assetdb_init(assetdb_t *db) {
    assetdb_status_t st = exec_sql(db, "PRAGMA foreign_keys=ON;");
    if (st != KV_OK) return st;

    st = exec_sql(db,
                  "CREATE TABLE IF NOT EXISTS kv ("
                  "k INTEGER NOT NULL,"
                  "v TEXT NOT NULL,"
                  "PRIMARY KEY(k)"
                  ") WITHOUT ROWID;"
    );
    if (st != KV_OK) return st;

    st = exec_sql(db,
                  "CREATE TABLE IF NOT EXISTS files ("
                  "hash   INTEGER NOT NULL,"
                  "name   TEXT,"
                  "size   INTEGER NOT NULL,"
                  "parent INTEGER NOT NULL DEFAULT 0,"
                  "PRIMARY KEY(hash)"
                  ") WITHOUT ROWID;"
                  "CREATE INDEX IF NOT EXISTS idx_files_parent ON files(parent);"
    );
    if (st != KV_OK) return st;

    // language=sqlite
    const StmtSpec stmts[] = {
        {&db->kv_put, "INSERT OR REPLACE INTO kv(k, v) VALUES(?1, ?2);"},
        {&db->kv_get, "SELECT v FROM kv WHERE k=?1;"},
        {&db->kv_del, "DELETE FROM kv WHERE k=?1;"},
        {&db->files_search, "SELECT name FROM files WHERE name LIKE ?1 ESCAPE '\\';"},
        {&db->files_put, "INSERT OR REPLACE INTO files(hash, name, size, parent) VALUES(?1, ?2, ?3, ?4)"},
        {&db->files_get, "SELECT name, size, parent FROM files WHERE hash=?1;"},
        {&db->files_del, "DELETE FROM files WHERE hash=?1;"},
    };

    return prepare_many(db, stmts, sizeof(stmts) / sizeof(stmts[0]));
}

assetdb_status_t assetdb_open(assetdb_t **out_db, const char *path) {
    if (!out_db || !path) return KV_EINVAL;
    *out_db = NULL;

    assetdb_t *db = (assetdb_t *) mp_calloc(1, sizeof(assetdb_t));
    if (!db) return KV_ENOMEM;

    const int rc = sqlite3_open_v2(path, &db->db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (rc != SQLITE_OK) {
        const assetdb_status_t st = set_err(db, rc, db->db ? sqlite3_errmsg(db->db) : "sqlite3_open_v2 failed");
        if (db->db) sqlite3_close(db->db);
        if (db->last_err) sqlite3_free(db->last_err);
        mp_free(db);
        return st;
    }

    const assetdb_status_t st = assetdb_init(db);
    if (st != KV_OK) {
        if (db->db) sqlite3_close(db->db);
        if (db->last_err) sqlite3_free(db->last_err);
        mp_free(db);
        return st;
    }

    *out_db = db;
    return KV_OK;
}

void assetdb_close(assetdb_t *db) {
    if (!db) return;

    stmt_finalize(&db->kv_put);
    stmt_finalize(&db->kv_get);
    stmt_finalize(&db->kv_del);

    stmt_finalize(&db->files_search);
    stmt_finalize(&db->files_put);
    stmt_finalize(&db->files_get);
    stmt_finalize(&db->files_del);

    if (db->last_err) sqlite3_free(db->last_err);
    if (db->db) sqlite3_close(db->db);

    mp_free(db);
}

/* kv */

assetdb_status_t assetdb_kv_put_u64(assetdb_t *db, const uint64_t key, const char *value) {
    if (!db || !value) return KV_EINVAL;
    stmt_reset(db->kv_put);

    assetdb_status_t st = bind_u64_i64(db, db->kv_put, 1, key);
    if (st != KV_OK) return st;

    const int rc = sqlite3_bind_text(db->kv_put, 2, value, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) return set_err(db, rc, sqlite3_errmsg(db->db));

    return stmt_step_done(db, db->kv_put);
}

assetdb_status_t assetdb_kv_get_u64_view(assetdb_t *db, const uint64_t key, const char **out, size_t *out_len) {
    if (!db || !out) return KV_EINVAL;
    *out = NULL;
    if (out_len) *out_len = 0;

    stmt_reset(db->kv_get);

    const assetdb_status_t st = bind_u64_i64(db, db->kv_get, 1, key);
    if (st != KV_OK) return st;

    const int rc = sqlite3_step(db->kv_get);
    if (rc == SQLITE_ROW) {
        const unsigned char *txt = sqlite3_column_text(db->kv_get, 0);
        const int n = sqlite3_column_bytes(db->kv_get, 0);
        *out = (const char *) txt;
        if (out_len) *out_len = (size_t) (n > 0 ? n : 0);
        return KV_OK;
    }
    if (rc == SQLITE_DONE) return KV_NOTFOUND;
    return set_err(db, rc, sqlite3_errmsg(db->db));
}

assetdb_status_t assetdb_kv_del_u64(assetdb_t *db, const uint64_t key) {
    if (!db) return KV_EINVAL;
    stmt_reset(db->kv_del);

    assetdb_status_t st = bind_u64_i64(db, db->kv_del, 1, key);
    if (st != KV_OK) return st;

    const int rc = sqlite3_step(db->kv_del);
    if (rc != SQLITE_DONE) return set_err(db, rc, sqlite3_errmsg(db->db));

    return (sqlite3_changes(db->db) > 0) ? KV_OK : KV_NOTFOUND;
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

static void *grow_array(void *p, const size_t elem, size_t *cap, const size_t need) {
    if (*cap >= need) return p;
    size_t c = (*cap ? *cap : 8);
    while (c < need) c *= 2;
    void *np = mp_realloc(p, c * elem);
    if (!np) return NULL;
    *cap = c;
    return np;
}

assetdb_status_t assetdb_files_search(assetdb_t *db, const char *pattern, char ***out, uint32_t *out_count) {
    if (!db || !pattern || !out) return KV_EINVAL;
    *out = NULL;
    if (out_count) *out_count = 0;

    stmt_reset(db->files_search);

    int rc = sqlite3_bind_text(db->files_search, 1, pattern, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) return set_err(db, rc, sqlite3_errmsg(db->db));

    size_t cap = 0;
    size_t count = 0;
    char **res = NULL;

    while ((rc = sqlite3_step(db->files_search)) == SQLITE_ROW) {
        const unsigned char *txt = sqlite3_column_text(db->files_search, 0);
        if (!txt) continue;

        res = (char **) grow_array(res, sizeof(char *), &cap, count + 2);
        if (!res) return KV_ENOMEM;

        const size_t n = strlen((const char *) txt) + 1;
        res[count] = (char *) mp_malloc(n);
        if (!res[count]) {
            for (size_t i = 0; i < count; i++)
                mp_free(res[i]);
            mp_free(res);
            return KV_ENOMEM;
        }
        memcpy(res[count], txt, n);
        count++;
    }

    if (rc != SQLITE_DONE) {
        for (size_t i = 0; i < count; i++)
            mp_free(res[i]);
        mp_free(res);
        return set_err(db, rc, sqlite3_errmsg(db->db));
    }

    if (res) res[count] = NULL;
    *out = res;
    if (out_count) *out_count = (uint32_t) count;
    return KV_OK;
}

void assetdb_vp_search_free(char **list) {
    if (!list) return;
    for (size_t i = 0; list[i]; i++)
        mp_free(list[i]);
    mp_free(list);
}

/* files */

assetdb_status_t assetdb_files_put(assetdb_t *db, const uint64_t hash, const char *name_nullable,
                                   const uint64_t size, const uint64_t parent_hash) {
    if (!db) return KV_EINVAL;
    stmt_reset(db->files_put);

    assetdb_status_t st;
    if ((st = bind_u64_i64(db, db->files_put, 1, hash)) != KV_OK) return st;
    if ((st = bind_text_nullable(db, db->files_put, 2, name_nullable)) != KV_OK) return st;
    if ((st = bind_u64_i64(db, db->files_put, 3, size)) != KV_OK) return st;
    if ((st = bind_u64_i64(db, db->files_put, 4, parent_hash)) != KV_OK) return st;

    return stmt_step_done(db, db->files_put);
}

assetdb_status_t assetdb_files_get_view(assetdb_t *db, const uint64_t hash,
                                        const char **out_name, size_t *out_name_len,
                                        uint64_t *out_size, uint64_t *out_parent_hash) {
    if (!db || !out_parent_hash) return KV_EINVAL;

    if (out_name) *out_name = NULL;
    if (out_name_len) *out_name_len = 0;
    if (out_size) *out_size = 0;
    *out_parent_hash = 0;

    stmt_reset(db->files_get);

    assetdb_status_t st = bind_u64_i64(db, db->files_get, 1, hash);
    if (st != KV_OK) return st;

    const int rc = sqlite3_step(db->files_get);
    if (rc == SQLITE_ROW) {
        if (out_name) {
            const unsigned char *txt = sqlite3_column_text(db->files_get, 0);
            const int n = sqlite3_column_bytes(db->files_get, 0);
            *out_name = txt ? (const char *) txt : NULL;
            if (out_name_len) *out_name_len = (size_t) (n > 0 ? n : 0);
        }

        const sqlite3_int64 sz = sqlite3_column_int64(db->files_get, 1);
        const sqlite3_int64 p = sqlite3_column_int64(db->files_get, 2);
        if (sz < 0 || p < 0) return KV_EINVAL;

        if (out_size) *out_size = (uint64_t) sz;
        *out_parent_hash = (uint64_t) p;
        return KV_OK;
    }
    if (rc == SQLITE_DONE) return KV_NOTFOUND;
    return set_err(db, rc, sqlite3_errmsg(db->db));
}

assetdb_status_t assetdb_files_del(assetdb_t *db, const uint64_t hash) {
    if (!db) return KV_EINVAL;
    stmt_reset(db->files_del);

    assetdb_status_t st = bind_u64_i64(db, db->files_del, 1, hash);
    if (st != KV_OK) return st;

    const int rc = sqlite3_step(db->files_del);
    if (rc != SQLITE_DONE) return set_err(db, rc, sqlite3_errmsg(db->db));

    return (sqlite3_changes(db->db) > 0) ? KV_OK : KV_NOTFOUND;
}

/* migration: vparent -> files (size = 0, name = basename(path) or NULL)
   Inserts only missing hashes into files, does not overwrite existing entries. */
assetdb_status_t assetdb_migrate_vparent_to_files(assetdb_t *db) {
    if (!db) return KV_EINVAL;

    assetdb_status_t st = exec_sql(db, "BEGIN IMMEDIATE;");
    if (st != KV_OK) return st;

    sqlite3_stmt *sel = NULL;
    st = prepare_stmt(db, &sel, "SELECT id, parent, path FROM vparent;");
    if (st != KV_OK) {
        exec_sql(db, "ROLLBACK;");
        return st;
    }

    sqlite3_stmt *ins = NULL;
    st = prepare_stmt(db, &ins,
                      "INSERT INTO files(hash, name, size, parent) VALUES(?1, ?2, 0, ?3) "
                      "ON CONFLICT(hash) DO NOTHING;"
    );
    if (st != KV_OK) {
        sqlite3_finalize(sel);
        exec_sql(db, "ROLLBACK;");
        return st;
    }

    int rc;
    while ((rc = sqlite3_step(sel)) == SQLITE_ROW) {
        const sqlite3_int64 id = sqlite3_column_int64(sel, 0);
        const sqlite3_int64 p = sqlite3_column_int64(sel, 1);
        const unsigned char *path = sqlite3_column_text(sel, 2);

        if (id < 0 || p < 0) {
            sqlite3_finalize(ins);
            sqlite3_finalize(sel);
            exec_sql(db, "ROLLBACK;");
            return KV_EINVAL;
        }

        const char *name = path_basename((const char *) path);

        stmt_reset(ins);
        if ((st = bind_u64_i64(db, ins, 1, (uint64_t) id)) != KV_OK) goto fail;
        if ((st = bind_text_nullable(db, ins, 2, name)) != KV_OK) goto fail;
        if ((st = bind_u64_i64(db, ins, 3, (uint64_t) p)) != KV_OK) goto fail;

        rc = sqlite3_step(ins);
        if (rc != SQLITE_DONE) {
            st = set_err(db, rc, sqlite3_errmsg(db->db));
            goto fail;
        }
    }

    if (rc != SQLITE_DONE) {
        st = set_err(db, rc, sqlite3_errmsg(db->db));
        goto fail;
    }

    sqlite3_finalize(ins);
    sqlite3_finalize(sel);

    st = exec_sql(db, "COMMIT;");
    if (st != KV_OK) {
        exec_sql(db, "ROLLBACK;");
        return st;
    }
    return KV_OK;

fail:
    sqlite3_finalize(ins);
    sqlite3_finalize(sel);
    exec_sql(db, "ROLLBACK;");
    return st;
}

/* errors */

const char *assetdb_last_error(const assetdb_t *db) {
    return (db && db->last_err) ? db->last_err : NULL;
}

int assetdb_last_sqlite_code(const assetdb_t *db) {
    return db ? db->last_sqlite : 0;
}
