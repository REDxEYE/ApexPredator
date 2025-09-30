// Created by RED on 28.09.2025.

#include "apex/package/archives.h"

#include <string.h>

#include "utils/lookup3.h"
#include "utils/path.h"

static const String tab_ext= {.buffer = ".tab", .size = 4, .capacity = 4, .can_be_moved = 0};

void Archives_init(Archives *ars, String *game_root) {
    DA_init(&ars->archives, Archive, 4);
    DynamicArray_Path archive_paths = {};

    Path_rglob(game_root, &tab_ext, &archive_paths);
    for (uint32 i = 0; i < archive_paths.count; ++i) {
        Archive *ar = DA_append_get(&ars->archives);
        Archive_open(ar, DA_at(&archive_paths, i));
    }
    DA_free_with_inner(&archive_paths, {
        String_free(it);
    });
}

void Archives_free(Archives *ars) {
    for (uint32 i = 0; i < ars->archives.count; ++i) {
        Archive_free(&ars->archives.items[i]);
    }
    DA_free(&ars->archives);
}

bool Archives_find_hash(Archives *ars, uint32 hash, MemoryBuffer *mb) {
    for (uint32 i = 0; i < ars->archives.count; ++i) {
        Archive *ar = &ars->archives.items[i];
        if (Archive_get_data(ar, hash, mb)) {
            printf("[INFO]: File with hash %08X found in archive \"%s\"\n", hash, String_data(&ar->arc_path));
            return true;
        }
    }
    printf("[ERROR]: File with hash %08X not found in any archive\n", hash);
    return false;
}



bool Archives_find_path(Archives *ars, const String *path, MemoryBuffer *mb) {
    uint32 hash = path_hash(String_data(path));
    return Archives_find_hash(ars, hash, mb);
}

uint32 path_hash(const char *str) {
    uint32 hash_lo = 0;
    uint32 hash_hi = 0;
    hashlittle2(str, (uint32) strlen(str), &hash_lo, &hash_hi);
    return hash_lo;
}
