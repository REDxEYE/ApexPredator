// Created by RED on 18.01.2026.
#include <stdio.h>
#include <memory.h>

#include "int_def.h"
#include "platform/memory_profiling.h"
#include "platform/memory_tracker.h"

void *tracy_xmalloc(const size_t n, const char *file, uint32_t line, const char *func) {
    static char msg_buf[1024];
    static uint32 msg_len;
    void *p = malloc(n);
    if (p) {
        msg_len = snprintf(msg_buf, sizeof(msg_buf), "Malloc p=0x%p, n=%llu", p, n);
        fprintf(stderr, "malloc(0x%p, %llu)\n", p, n);
        TracyCMessage(msg_buf, msg_len);
        TracyCAlloc(p, n);
    }
    return p;
}

void tracy_xfree(void *p, const char *file, uint32_t line, const char *func) {
    static char msg_buf[1024];
    static uint32 msg_len;
    if (p) {
        const size_t size = _msize(p);
        msg_len = snprintf(msg_buf, sizeof(msg_buf), "Free ptr=0x%p", p);
        fprintf(stderr, "free(0x%p, %llu)\n", p, size);
        TracyCMessage(msg_buf, msg_len);
        TracyCFree(p);
    }
    free(p);
}

void *tracy_xrealloc(void *p, size_t n, const char *file, const uint32_t line, const char *func) {
    void *np = mp_malloc(n);
    if (np && p) {
        const size_t old_size = _msize(p);
        const size_t copy_size = old_size < n ? old_size : n;
        memmove(np, p, copy_size);
        mp_free(p);
    }
    return np;


    // static char msg_buf[1024];
    // static uint32 msg_len;
    // if (p==NULL) {
    //     return tracy_xmalloc(n, file, line, func);
    // }
    // if (n == 0) {
    //     msg_len = snprintf(msg_buf, sizeof(msg_buf), "Realloc(0) free ptr=0x%p", p);
    //     fprintf(stderr, "realloc(0x%p, 0)\n", p);
    //     TracyCMessage(msg_buf, msg_len);
    //     TracyCFreeN(p, func);
    //     free(p);
    //     return NULL;
    // }
    //
    // void *np = realloc(p, n);
    // if (!np) return NULL; // old block still valid, still tracked
    //
    // if (np == p) {
    //     // resized in place: tell Tracy "free then alloc" for same ptr, in order
    //     msg_len = snprintf(msg_buf, sizeof(msg_buf), "Realloc inplace resize free ptr=0x%p, n=%llu", p, n);
    //     fprintf(stderr, "realloc(0x%p, %llu) inplace free\n", p, n);
    //     TracyCMessage(msg_buf, msg_len);
    //     TracyCFreeN(p, func);
    //     msg_len = snprintf(msg_buf, sizeof(msg_buf), "Realloc inplace resize alloc ptr=0x%p, n=%llu", np, n);
    //     fprintf(stderr, "realloc(0x%p, %llu) inplace alloc\n", p, n);
    //     TracyCMessage(msg_buf, msg_len);
    //     TracyCAllocN(np, n, func);
    // } else {
    //     msg_len = snprintf(msg_buf, sizeof(msg_buf), "Realloc free old ptr=0x%p, new ptr=0x%p, n=%llu", p, np, n);
    //     fprintf(stderr, "realloc(0x%p, %llu) free new ptr %p\n", p, n, np);
    //     TracyCMessage(msg_buf, msg_len);
    //     TracyCFreeN(p, func);
    //     msg_len = snprintf(msg_buf, sizeof(msg_buf), "Realloc alloc new ptr=0x%p, n=%llu", np, n);
    //     fprintf(stderr, "realloc(0x%p, %llu) alloc new ptr %p\n", p, n, np);
    //     TracyCMessage(msg_buf, msg_len);
    //     TracyCAllocN(np, n, func);
    // }
    // return np;
}

void *tracy_xcalloc(const size_t count, const size_t size, const char *file, uint32_t line, const char *func) {
    static char msg_buf[1024];
    static uint32 msg_len;
    const size_t total = count * size;
    void *p = calloc(count, size);
    if (p) {
        msg_len = snprintf(msg_buf, sizeof(msg_buf), "Calloc p=0x%p, count=%llu, size=%llu", p, count, size);
        fprintf(stderr, "calloc(0x%p, %llu)\n", p, total);
        TracyCMessage(msg_buf, msg_len);
        TracyCAllocN(p, total, func);
    }
    return p;
}

static MpTrack *g_mp;

static void mp_report(const MpTrackIssue *is) {
    if (!is || is->kind == MP_ISSUE_NONE) return;

    if (is->kind == MP_ISSUE_DOUBLE_ALLOC) {
        fprintf(stderr, "MP: DOUBLE_ALLOC ptr=%p old_size=%zu new_size=%zu\n"
                "  first:  %s:%u (%s)\n"
                "  second: %s:%u (%s)\n",
                is->ptr, is->size_a, is->size_b,
                is->first.file, is->first.line, is->first.func,
                is->second.file, is->second.line, is->second.func
        );
    } else if (is->kind == MP_ISSUE_FREE_WITHOUT_ALLOC) {
        fprintf(stderr, "MP: FREE_WITHOUT_ALLOC ptr=%p at %s:%u (%s)\n",
                is->ptr, is->second.file, is->second.line, is->second.func
        );
    }
}

void mp_init(void) {
#ifdef ALLOC_DEBUG
    if (!g_mp) g_mp = mp_track_create((size_t) 1 << 16);
#endif
}

void mp_shutdown(void) {
#ifdef ALLOC_DEBUG
    mp_track_destroy(g_mp);
    g_mp = NULL;
#endif
}

void *tracy_xmalloc_dbg(size_t n, const char *file, const uint32_t line, const char *func) {
    void *p = malloc(n);
    if (p) {
        MpTrackIssue is = {0};
        if (!mp_track_alloc(g_mp, p, n, (MpSite){file, func, line}, &is)) mp_report(&is);
        // else {
        TracyCAlloc(p, n);
        // }
    }
    return p;
}

void tracy_xfree_dbg(void *p, const char *file, const uint32_t line, const char *func) {
    if (p) {
        MpTrackIssue is = {0};
        if (!mp_track_free(g_mp, p, (MpSite){file, func, line}, &is)) mp_report(&is);
        // else {
        TracyCFree(p);
        // }
    }

    free(p);
}

void *tracy_xrealloc_dbg(void *p, const size_t n, const char *file, const uint32_t line, const char *func) {
    const MpSite site = (MpSite){file, func, line};

    if (!p) {
        void *np = malloc(n);
        if (np) {
            MpTrackIssue is = {0};
            if (!mp_track_alloc(g_mp, np, n, site, &is)) mp_report(&is);
            // else {
            TracyCAlloc(np, n);
            // }
        }
        return np;
    }

    if (n == 0) {
        MpTrackIssue is = {0};
        if (!mp_track_free(g_mp, p, site, &is)) mp_report(&is);
        // else {
        TracyCFree(p);
        // }
        free(p);
        return NULL;
    }

    void *np = realloc(p, n);
    if (!np) return NULL;

    if (np == p) {
        MpTrackIssue is = {0};
        if (!mp_track_resize(g_mp, p, n, site, &is)) mp_report(&is);
        // else {
        TracyCFree(p);
        TracyCAlloc(p, n);
        // }
        return p;
    }

    MpTrackIssue is = {0};
    if (!mp_track_free(g_mp, p, site, &is)) mp_report(&is);
    // else {
    TracyCFree(p);
    // }

    is = (MpTrackIssue){0};
    if (!mp_track_alloc(g_mp, np, n, site, &is)) mp_report(&is);
    // else {
    TracyCAlloc(np, n);
    // }

    return np;
}

void *tracy_xcalloc_dbg(const size_t count, const size_t size, const char *file, const uint32_t line,
                        const char *func) {
    void *p = calloc(count, size);
    if (p) {
        const size_t n = count * size;
        MpTrackIssue is = {0};
        if (!mp_track_alloc(g_mp, p, n, (MpSite){file, func, line}, &is))
            mp_report(&is);
    }
    TracyCAlloc(p, count * size);
    return p;
}
