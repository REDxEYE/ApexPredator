// Created by RED on 07.10.2025.

#include "apex/hashes.h"
#include "apex/adf/adf_types.h"
#include "utils/dynamic_array.h"
#include "utils/dynamic_insert_only_map.h"
#include "platform/common_arrays.h"

DYNAMIC_INSERT_ONLY_INT_MAP_STRUCT(charPtr, charPtr);

const static DynamicInsertOnlyIntMap_charPtr names_map = {
        .keys = {
                .items = STI_ADF_TYPES_hash_strings_hash,
                .item_size = 8,
                .capacity = sizeof(STI_ADF_TYPES_hash_strings_hash) / sizeof(uint64),
                .statically_allocated = 1,
                .heap_allocated = 0,
                .count = sizeof(STI_ADF_TYPES_hash_strings_hash) / sizeof(uint64),
        },
        .values = {
                .items = (char **) STI_ADF_TYPES_hash_strings_string,
                .item_size = 8,
                .capacity = sizeof(STI_ADF_TYPES_hash_strings_string) / sizeof(char *),
                .statically_allocated = 1,
                .heap_allocated = 0,
                .count = sizeof(STI_ADF_TYPES_hash_strings_string) / sizeof(char *),
        },
};


typedef struct {
    uint32 hash;
    const char *name;
} KnownHashToName;

DYNAMIC_ARRAY_STRUCT(KnownHashToName, KnownHashToName);

const static KnownHashToName names2[] = {
#include "apex/hashes.inc"
};

const static uint32 names2_len = sizeof(names2) / sizeof(names2[0]);

String *find_name(uint32 key) {
    const char *real_name = NULL;
    // hashes are sorted, use binary search
    int left = 0;
    int right = names2_len - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (names2[mid].hash == key) {
            real_name = names2[mid].name;
            break;
        } else if (names2[mid].hash < key) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    if (real_name == NULL) {
        void **res = (DM_get(&names_map, key));
        real_name = res != NULL ? *res : NULL;
    }
    if (real_name != NULL) {
        return String_new_from_cstr(real_name);
    }
    String *tmp = String_new(64);
    String_append_format(tmp, "0x%08X", key);

    return tmp;
}
