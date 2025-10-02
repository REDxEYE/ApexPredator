// Created by RED on 02.10.2025.

#ifndef APEXPREDATOR_ARCHIVE_H
#define APEXPREDATOR_ARCHIVE_H
#include <assert.h>

#include "int_def.h"
#include "utils/string.h"
#include "utils/dynamic_array.h"
#include "utils/lookup3.h"
#include "utils/buffer/memory_buffer.h"

typedef struct Archive Archive;

typedef struct {
    Archive *archive;
    const String *path;
    uint32 path_hash;
    uint32 size;
} ArchiveEntry;

DYNAMIC_ARRAY_STRUCT(ArchiveEntry, ArchiveEntry);

// Interface function typedefs
// Has file path/hash
typedef bool (*ArchiveHasFileFn)(const Archive *archive, const String *path);

typedef bool (*ArchiveHasFileByHashFn)(const Archive *archive, uint32 hash);

// Get file data
typedef bool (*ArchiveGetFileFn)(Archive *archive, const String *path, MemoryBuffer *out);

typedef bool (*ArchiveGetFileByHashFn)(Archive *archive, uint32 hash, MemoryBuffer *out);

// Get all entries from archive
typedef void (*ArchiveGetAllEntriesFn)(const Archive *archive, DynamicArray_ArchiveEntry *entries);

// Get archive name
typedef const String * (*ArchiveGetNameFn)(const Archive *archive);

// Print all of the files
typedef void (*ArchivePrintAllFilesFn)(const Archive *archive);

// Free function
typedef void (*ArchiveFreeFn)(Archive *archive);

typedef struct ArchiveInterface {
    ArchiveHasFileFn has_file;
    ArchiveHasFileByHashFn has_file_by_hash;
    ArchiveGetFileFn get_file;
    ArchiveGetFileByHashFn get_file_by_hash;
    ArchiveGetAllEntriesFn get_all_entries;
    ArchiveGetNameFn get_name;
    ArchivePrintAllFilesFn print_all_files;
    ArchiveFreeFn free;
} ArchiveInterface;

typedef struct Archive {
    struct ArchiveInterface;
} Archive;

// Shortcuts
static inline void Archive_free(Archive *archive) {
    assert(archive!=NULL);
    if (archive->free) {
        archive->free(archive);
    }
}

static inline bool Archive_has_file(const Archive *archive, const String *path) {
    assert(archive!=NULL);
    if (archive->has_file) {
        return archive->has_file(archive, path);
    }
    return false;
}

static inline bool Archive_has_file_by_hash(const Archive *archive, uint32 hash) {
    assert(archive!=NULL);
    if (archive->has_file_by_hash) {
        return archive->has_file_by_hash(archive, hash);
    }
    return false;
}

static inline bool Archive_get_file(Archive *archive, const String *path, MemoryBuffer *out) {
    assert(archive!=NULL);
    if (archive->get_file) {
        return archive->get_file(archive, path, out);
    }
    return false;
}

static inline bool Archive_get_file_by_hash(Archive *archive, uint32 hash, MemoryBuffer *out) {
    assert(archive!=NULL);
    if (archive->get_file_by_hash) {
        return archive->get_file_by_hash(archive, hash, out);
    }
    return false;
}

static inline void Archive_get_all_entries(const Archive *archive, DynamicArray_ArchiveEntry *entries) {
    assert(archive!=NULL);
    if (archive->get_all_entries) {
        archive->get_all_entries(archive, entries);
    }
}

static inline const String *Archive_get_name(const Archive *archive) {
    assert(archive!=NULL);
    if (archive->get_name) {
        return archive->get_name(archive);
    }
    return NULL;
}
static inline void Archive_print_files(const Archive *archive) {
    assert(archive!=NULL);
    if (archive->print_all_files) {
        archive->print_all_files(archive);
    }
}

static inline uint32 path_hash(const String *str) {
    uint32 hash_lo = 0;
    uint32 hash_hi = 0;
    hashlittle2(String_data(str), str->size, &hash_lo, &hash_hi);
    return hash_lo;
}


#endif //APEXPREDATOR_ARCHIVE_H
