// Created by RED on 18.01.2026.

#include "platform/memory_tracker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MpTrack {
    MpEntry* entries;
    size_t   cap;
    size_t   size;
    size_t   used;     /* includes tombstones */
};

static void* const MP_EMPTY = NULL;
static void* const MP_TOMB  = (void*)1;

static size_t mp_hash_ptr(const void* p) {
    uintptr_t x = (uintptr_t)p;
#if UINTPTR_MAX > 0xffffffffu
    x ^= x >> 33;
    x *= (uintptr_t)0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= (uintptr_t)0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
#else
    x ^= x >> 16;
    x *= (uintptr_t)0x7feb352dU;
    x ^= x >> 15;
    x *= (uintptr_t)0x846ca68bU;
    x ^= x >> 16;
#endif
    return (size_t)x;
}

static size_t mp_next_pow2(size_t v) {
    if (v < 8) return 8;
    v--;
    for (size_t s = 1; s < sizeof(size_t) * 8; s <<= 1) v |= v >> s;
    return v + 1;
}

static MpTrack* mp_alloc_track(size_t cap) {
    MpTrack* t = (MpTrack*)calloc(1, sizeof(MpTrack));
    if (!t) return NULL;
    t->cap = cap;
    t->entries = (MpEntry*)calloc(cap, sizeof(MpEntry));
    if (!t->entries) { free(t); return NULL; }
    return t;
}

MpTrack* mp_track_create(const size_t initial_capacity_pow2) {
    const size_t cap = initial_capacity_pow2 ? mp_next_pow2(initial_capacity_pow2) : (size_t)1 << 16;
    return mp_alloc_track(cap);
}

void mp_track_destroy(MpTrack* t) {
    if (!t) return;
    // Report still active allocations
    for (int i = 0; i < t->size; ++i) {
        const MpEntry* e = &t->entries[i];
        if (e->key != MP_EMPTY && e->key != MP_TOMB) {
            fprintf(stderr, "MP: LEAK ptr=%p size=%llu allocated at %s:%u (%s)\n",
                    e->key, e->size, e->site.file, e->site.line, e->site.func
            );
        }
    }

    free(t->entries);
    free(t);
}

size_t mp_track_size(const MpTrack* t) { return t ? t->size : 0; }
size_t mp_track_capacity(const MpTrack* t) { return t ? t->cap : 0; }

static void mp_rehash(MpTrack* t, const size_t new_cap) {
    MpEntry* old = t->entries;
    const size_t old_cap = t->cap;

    t->entries = (MpEntry*)calloc(new_cap, sizeof(MpEntry));
    t->cap = new_cap;
    t->size = 0;
    t->used = 0;

    for (size_t i = 0; i < old_cap; i++) {
        void* k = old[i].key;
        if (k == MP_EMPTY || k == MP_TOMB) continue;

        size_t mask = t->cap - 1;
        size_t idx = mp_hash_ptr(k) & mask;
        while (t->entries[idx].key != MP_EMPTY) idx = (idx + 1) & mask;

        t->entries[idx] = old[i];
        t->size++;
        t->used++;
    }

    free(old);
}

static void mp_maybe_grow(MpTrack* t) {
    /* grow when used (incl tomb) > ~70% */
    if ((t->used + 1) * 10 < t->cap * 7) return;
    mp_rehash(t, t->cap * 2);
}

MpEntry* mp_find_slot(MpTrack* t, void* key, int* found) {
    size_t mask = t->cap - 1;
    size_t idx = mp_hash_ptr(key) & mask;
    size_t first_tomb = (size_t)-1;

    for (;;) {
        void* k = t->entries[idx].key;

        if (k == MP_EMPTY) {
            *found = 0;
            if (first_tomb != (size_t)-1) return &t->entries[first_tomb];
            return &t->entries[idx];
        }

        if (k == MP_TOMB) {
            if (first_tomb == (size_t)-1) first_tomb = idx;
        } else if (k == key) {
            *found = 1;
            return &t->entries[idx];
        }

        idx = (idx + 1) & mask;
    }
}

static void mp_fill_issue(MpTrackIssue* out, MpTrackIssueKind kind, void* p,
                          size_t sa, size_t sb, MpSite a, MpSite b) {
    if (!out) return;
    out->kind = kind;
    out->ptr = p;
    out->size_a = sa;
    out->size_b = sb;
    out->first = a;
    out->second = b;
}

int mp_track_resize(MpTrack* t, void* p, size_t new_size, MpSite where, MpTrackIssue* out_issue) {
    if (!t || !p) return 1;

    int found = 0;
    MpEntry* e = mp_find_slot(t, p, &found);
    if (!found) {
        mp_fill_issue(out_issue, MP_ISSUE_FREE_WITHOUT_ALLOC, p, 0, 0, (MpSite){0}, where);
        return 0;
    }
    e->size = new_size;
    e->site = where;
    return 1;
}

int mp_track_alloc(MpTrack* t, void* p, size_t n, MpSite where, MpTrackIssue* out_issue) {
    if (!t || !p) return 1;

    mp_maybe_grow(t);

    int found = 0;
    MpEntry* e = mp_find_slot(t, p, &found);

    if (found) {
        mp_fill_issue(out_issue, MP_ISSUE_DOUBLE_ALLOC, p, e->size, n, e->site, where);
        return 0;
    }

    if (e->key == MP_EMPTY) t->used++;
    e->key = p;
    e->size = n;
    e->site = where;
    t->size++;
    return 1;
}

int mp_track_free(MpTrack* t, void* p, MpSite where, MpTrackIssue* out_issue) {
    if (!t || !p) return 1;

    int found = 0;
    MpEntry* e = mp_find_slot(t, p, &found);

    if (!found) {
        mp_fill_issue(out_issue, MP_ISSUE_FREE_WITHOUT_ALLOC, p, 0, 0, (MpSite){0}, where);
        return 0;
    }

    e->key = MP_TOMB;
    t->size--;
    return 1;
}