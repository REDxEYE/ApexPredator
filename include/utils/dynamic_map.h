// Created by RED on 19.09.2025.

#ifndef APEXPREDATOR_DYNAMIC_INSERT_ONLY_MAP_H
#define APEXPREDATOR_DYNAMIC_INSERT_ONLY_MAP_H

#include "utils/dynamic_array.h"

DYNAMIC_ARRAY_STRUCT(uint64, DM_Key);
DYNAMIC_ARRAY_STRUCT(uint32, DM_freeList);

typedef struct {
    DynamicArray_DM_Key keys;   /* uint64 keys indexed by value_index (u32 keys are stored as zero-extended) */
    DynamicArray__Base  values; /* raw value bytes indexed by value_index */
    DynamicArray__Base  free;   /* uint32 freelist of reusable value_index */

    uint32 *table;              /* slot -> value_index, or special sentinels */
    uint32  mask;               /* table_size - 1 */
    uint32  used;               /* live entries */
    uint32  fill;               /* live + tombstones */
} DynamicIntMap__Base;


//Very specialized hash map, only support uint64 keys and keys are assumed to be unique
#define DYNAMIC_INT_MAP_STRUCT(value_type, name) \
    typedef struct { \
        DynamicArray_DM_Key keys; \
        DynamicArray_##value_type values; \
        DynamicArray_DM_freeList free; \
        uint32 *table; \
        uint32  mask; \
        uint32  used; \
        uint32  fill; \
    } DynamicIntMap_##name

void DM_init_(DynamicIntMap__Base *dm, uint32 value_item_size, uint32 initial_capacity);

void DM_free_(DynamicIntMap__Base *dm);

void DM_clear_(DynamicIntMap__Base *dm);

void DM_reserve_(DynamicIntMap__Base *dm, uint32 capacity);

uint32 DM_count_(const DynamicIntMap__Base *dm);

void *DM_get_(const DynamicIntMap__Base *dm, uint64 key_hash);

void *DM_insert_(DynamicIntMap__Base *dm, uint64 key_hash);

bool DM_erase_(DynamicIntMap__Base *dm, uint64 key_hash);

void *DM_get_value_(const DynamicIntMap__Base *dm, uint32 value_index);

#define DM_init(dm, value_type, initial_capacity) \
    DM_init_((DynamicIntMap__Base*)(dm), (uint32)sizeof(value_type), (initial_capacity))

#define DM_free(dm) \
    DM_free_((DynamicIntMap__Base*)(dm))

#define DM_clear(dm) \
    DM_clear_((DynamicIntMap__Base*)(dm))

#define DM_reserve(dm, capacity) \
    DM_reserve_((DynamicIntMap__Base*)(dm), (capacity))

#define DM_count(dm) \
    DM_count_((DynamicIntMap__Base*)(dm))

#define DM_get(dm, key_hash_u64) \
    (void*)DM_get_((DynamicIntMap__Base*)(dm), (uint64)(key_hash_u64))

#define DM_insert(dm, key_hash_u64) \
    (void*)DM_insert_((DynamicIntMap__Base*)(dm), (uint64)(key_hash_u64))

#define DM_erase(dm, key_hash_u64) \
    DM_erase_((DynamicIntMap__Base*)(dm), (uint64)(key_hash_u64))

#define DM_get_value(dm, value_index) \
    (void*)DM_get_value_((DynamicIntMap__Base*)(dm), (uint32)(value_index))


#endif //APEXPREDATOR_DYNAMIC_INSERT_ONLY_MAP_H
