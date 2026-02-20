// Created by RED on 18.01.2026.

#ifndef APEXPREDATOR_MEMORY_TRACKER_H
#define APEXPREDATOR_MEMORY_TRACKER_H
#include <stddef.h>
#include <stdint.h>

#ifndef MP_MAX_FRAMES
#define MP_MAX_FRAMES 16
#endif

typedef struct MpSite {
    const char* file;
    const char* func;
    uint32_t    line;
} MpSite;

typedef enum MpTrackIssueKind {
    MP_ISSUE_NONE = 0,
    MP_ISSUE_DOUBLE_ALLOC,
    MP_ISSUE_FREE_WITHOUT_ALLOC,
} MpTrackIssueKind;

typedef struct MpTrackIssue {
    MpTrackIssueKind kind;
    void*            ptr;
    size_t           size_a;
    size_t           size_b;
    MpSite           first;
    MpSite           second;
} MpTrackIssue;

typedef struct MpEntry {
    void*  key;
    size_t size;
    MpSite site;
} MpEntry;

typedef struct MpTrack MpTrack;

MpTrack* mp_track_create(size_t initial_capacity_pow2); /* e.g. 1<<16 */
void     mp_track_destroy(MpTrack* t);

int      mp_track_resize(MpTrack* t, void* p, size_t new_size, MpSite where, MpTrackIssue* out_issue);
int      mp_track_alloc(MpTrack* t, void* p, size_t n, MpSite where, MpTrackIssue* out_issue);
int      mp_track_free (MpTrack* t, void* p, MpSite where, MpTrackIssue* out_issue);

size_t   mp_track_size(const MpTrack* t);
size_t   mp_track_capacity(const MpTrack* t);

MpEntry* mp_find_slot(MpTrack* t, void* key, int* found);

void* tracy_xmalloc_dbg(size_t n, const char* file, uint32_t line, const char* func);
void  tracy_xfree_dbg(void* p, const char* file, uint32_t line, const char* func);
void* tracy_xrealloc_dbg(void* p, size_t n, const char* file, uint32_t line, const char* func);

void mp_init(void);

void mp_shutdown(void);

/* Convenience macros to capture callsite */
#define MP_SITE() (MpSite){__FILE__, __func__, (uint32_t)__LINE__}
#endif //APEXPREDATOR_MEMORY_TRACKER_H