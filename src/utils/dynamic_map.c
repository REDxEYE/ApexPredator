// Created by RED on 19.09.2025.

#include "utils/dynamic_map.h"
#include "utils/dynamic_array.h"

#include "int_def.h"
#include <string.h>

#include "utils/memory_profiling.h"

enum { DM_EMPTY = 0xFFFFFFFFu, DM_TOMB = 0xFFFFFFFEu };

static uint64 dm_hash_u64(uint64 x) {
    /* splitmix64 */
    x += 0x9e3779b97f4a7c15ull;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
    return x ^ (x >> 31);
}

static uint32 dm_next_pow2(uint32 v) {
    if (v <= 2) return 2;
    v--;
    v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16;
    v++;
    return v;
}

static uint32 dm_table_size_for(const uint32 n) {
    /* keep load factor <= ~0.66 */
    const uint32 need = (n * 3u) / 2u + 8u;
    uint32 sz = dm_next_pow2(need);
    if (sz < 8) sz = 8;
    return sz;
}

static bool dm_table_alloc(DynamicIntMap__Base *dm, const uint32 sz) {
    dm->table = (uint32*)mp_malloc((size_t)sz * sizeof(uint32));
    if (!dm->table) return false;
    for (uint32 i = 0; i < sz; i++) dm->table[i] = DM_EMPTY;
    dm->mask = sz - 1;
    dm->used = 0;
    dm->fill = 0;
    return true;
}

static void dm_table_free(DynamicIntMap__Base *dm) {
    mp_free(dm->table);
    dm->table = NULL;
    dm->mask = 0;
    dm->used = 0;
    dm->fill = 0;
}

static bool dm_resize(DynamicIntMap__Base *dm, const uint32 new_sz) {
    uint32 *old = dm->table;
    const uint32 old_sz = dm->mask ? (dm->mask + 1) : 0;

    uint32 *tab = (uint32*)mp_malloc((size_t)new_sz * sizeof(uint32));
    if (!tab) return false;
    for (uint32 i = 0; i < new_sz; i++) tab[i] = DM_EMPTY;

    dm->table = tab;
    dm->mask = new_sz - 1;
    dm->fill = 0;

    for (uint32 i = 0; i < old_sz; i++) {
        const uint32 vi = old[i];
        if (vi == DM_EMPTY || vi == DM_TOMB) continue;

        const uint64 key = dm->keys.items[vi];
        uint32 pos = (uint32)dm_hash_u64(key) & dm->mask;
        while (dm->table[pos] != DM_EMPTY) pos = (pos + 1) & dm->mask;
        dm->table[pos] = vi;
        dm->fill++;
    }

    mp_free(old);
    return true;
}

static void dm_maybe_grow(DynamicIntMap__Base *dm, const uint32 add_fill) {
    const uint32 sz = dm->mask ? (dm->mask + 1) : 0;
    if (!sz) return;
    const uint32 nf = dm->fill + add_fill;
    if (nf * 3u >= sz * 2u) {
        uint32 want = dm_table_size_for(dm->used + 1);
        if (want < sz * 2u) want = sz * 2u;
        (void)dm_resize(dm, want);
    }
}

/* Probe: returns true if found, and gives slot in out_pos.
   out_free_pos is where to insert (first tomb or empty). */
static bool dm_probe(const DynamicIntMap__Base *dm, const uint64 key,
                            uint32 *out_pos, uint32 *out_free_pos) {
    uint32 pos = (uint32)dm_hash_u64(key) & dm->mask;
    uint32 first_tomb = UINT32_MAX;

    for (;;) {
        const uint32 vi = dm->table[pos];
        if (vi == DM_EMPTY) {
            *out_pos = pos;
            *out_free_pos = (first_tomb != UINT32_MAX) ? first_tomb : pos;
            return false;
        }
        if (vi == DM_TOMB) {
            if (first_tomb == UINT32_MAX) first_tomb = pos;
        } else {
            if (dm->keys.items[vi] == key) {
                *out_pos = pos;
                *out_free_pos = pos;
                return true;
            }
        }
        pos = (pos + 1) & dm->mask;
    }
}

/* value slot alloc: reuse freelist or append */
static uint32 dm_alloc_value_index(DynamicIntMap__Base *dm) {
    if (dm->free.count) {
        const uint32 *p = (uint32*)DA_at(&dm->free, dm->free.count - 1);
        const uint32 vi = *p;
        dm->free.count--;
        return vi;
    }
    /* append new key + value */
    uint64 *k = (uint64*)DA_append_get(&dm->keys);
    if (!k) return DM_EMPTY;
    void *v = DA_append_get_(&dm->values);
    if (!v) return DM_EMPTY;
    return dm->keys.count - 1;
}

/* ---- public API ---- */

void DM_init_(DynamicIntMap__Base* dm, const uint32 item_size, const uint32 initial_capacity) {
    memset(dm, 0, sizeof(*dm));
    DA_init(&dm->keys, uint64, initial_capacity);
    DA_init_(&dm->values, item_size, initial_capacity);
    DA_init(&dm->free, uint32, 1);

    const uint32 tsz = dm_table_size_for(initial_capacity ? initial_capacity : 8);
    (void)dm_table_alloc(dm, tsz);
}

uint32 DM_count_(const DynamicIntMap__Base* dm) {
    return dm ? dm->used : 0;
}

void DM_reserve_(DynamicIntMap__Base* dm, const uint32 capacity) {
    if (!dm) return;
    DA_reserve(&dm->keys, capacity);
    DA_reserve_(&dm->values, capacity);

    const uint32 want = dm_table_size_for(capacity);
    const uint32 cur = dm->mask ? (dm->mask + 1) : 0;
    if (!dm->table) { (void)dm_table_alloc(dm, want); return; }
    if (want > cur) (void)dm_resize(dm, want);
}

void* DM_get_value_(const DynamicIntMap__Base* dm, const uint32 index) {
    if (!dm) return NULL;
    if (index >= dm->values.count) return NULL;
    return (char*)dm->values.items + (size_t)index * dm->values.item_size;
}

/* returns pointer to value or NULL */
void* DM_get_(const DynamicIntMap__Base* dm, const uint64 key) {
    if (!dm || !dm->table) return NULL;
    uint32 pos, free_pos;
    if (!dm_probe(dm, key, &pos, &free_pos)) return NULL;
    const uint32 vi = dm->table[pos];
    return (char*)dm->values.items + (size_t)vi * dm->values.item_size;
}

/* put/insert: returns pointer to value (existing or new zeroed) */
void* DM_insert_(DynamicIntMap__Base* dm, const uint64 key_hash) {
    if (!dm || !dm->table) return NULL;

    uint32 pos, free_pos;
    if (dm_probe(dm, key_hash, &pos, &free_pos)) {
        const uint32 vi = dm->table[pos];
        return (char*)dm->values.items + (size_t)vi * dm->values.item_size;
    }

    dm_maybe_grow(dm, 1);

    /* re-probe after resize */
    if (dm_probe(dm, key_hash, &pos, &free_pos)) {
        const uint32 vi = dm->table[pos];
        return (char*)dm->values.items + (size_t)vi * dm->values.item_size;
    }

    const uint32 vi = dm_alloc_value_index(dm);
    if (vi == DM_EMPTY) return NULL;

    dm->keys.items[vi] = key_hash;

    void *v = (char*)dm->values.items + (size_t)vi * dm->values.item_size;
    memset(v, 0, dm->values.item_size);

    if (dm->table[free_pos] == DM_TOMB) {
        dm->table[free_pos] = vi;
    } else {
        dm->table[free_pos] = vi;
        dm->fill++;
    }
    dm->used++;
    return v;
}

/* delete: returns true if removed */
bool DM_erase_(DynamicIntMap__Base* dm, const uint64 key) {
    if (!dm || !dm->table) return false;

    uint32 pos, free_pos;
    if (!dm_probe(dm, key, &pos, &free_pos)) return false;

    const uint32 vi = dm->table[pos];
    dm->table[pos] = DM_TOMB;
    dm->used--;

    uint32 *fp = (uint32*)DA_append_get_(&dm->free);
    if (fp) *fp = vi;

    return true;
}

/* clear: keeps capacity, resets table to empty, drops freelist */
void DM_clear_(DynamicIntMap__Base* dm) {
    if (!dm || !dm->table) return;
    const uint32 sz = dm->mask + 1;
    for (uint32 i = 0; i < sz; i++) dm->table[i] = DM_EMPTY;
    dm->used = 0;
    dm->fill = 0;
    dm->free.count = 0;

    /* optional: keep keys/values buffers but reset counts */
    dm->keys.count = 0;
    dm->values.count = 0;
}

void DM_free_(DynamicIntMap__Base* dm) {
    if (!dm) return;
    dm_table_free(dm);
    DA_free(&dm->keys);
    DA_free_(&dm->values);
    DA_free_(&dm->free);
    memset(dm, 0, sizeof(*dm));
}