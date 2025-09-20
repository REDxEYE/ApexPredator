// Created by RED on 19.09.2025.

#ifndef APEXPREDATOR_DYNAMIC_INSERT_ONLY_MAP_H
#define APEXPREDATOR_DYNAMIC_INSERT_ONLY_MAP_H

#include <stddef.h>
#include "utils/dynamic_array.h"

DYNAMIC_ARRAY_STRUCT(uint32, DM_Key);

typedef struct {
    DynamicArray_DM_Key keys;
    DynamicArray__Base values;
} DynamicInsertOnlyIntMap__Base;


//Very specialized hash map, only support uint32 keys and keys are assumed to be unique
#define DYNAMIC_INSERT_ONLY_INT_MAP_STRUCT(value_type, name) \
    typedef struct { \
        DynamicArray_DM_Key keys; \
        DynamicArray_##value_type values; \
    } DynamicInsertOnlyIntMap_##name

#define DM_ASSERT_PTR_COMPAT(dm)                                                     \
    (                                                                                \
        (void)sizeof(char[(sizeof(*(dm))!=sizeof(DynamicInsertOnlyIntMap__Base))?-1:1]), \
        0                                                                            \
    )

void DM_init_(DynamicInsertOnlyIntMap__Base* dm, uint32 item_size, uint32 initial_capacity);
void* DM_insert_(DynamicInsertOnlyIntMap__Base* dm, uint32 key);
void* DM_get_(const DynamicInsertOnlyIntMap__Base* dm, uint32 key);
void* DM_get_value_(const DynamicInsertOnlyIntMap__Base* dm, uint32 index);
uint32 DM_count_(const DynamicInsertOnlyIntMap__Base* dm);

#define DM_init(dm, value_type, initial_capacity)                             \
    (DM_ASSERT_PTR_COMPAT(dm),                                               \
    DM_init_((DynamicInsertOnlyIntMap__Base*)(dm),                           \
    sizeof(value_type), (initial_capacity)))

#define DM_insert(dm, key)                                                    \
    (DM_ASSERT_PTR_COMPAT(dm),                                               \
    (void*)DM_insert_((DynamicInsertOnlyIntMap__Base*)(dm), (key)))

#define DM_get(dm, key)                                                       \
    (DM_ASSERT_PTR_COMPAT(dm),                                               \
    (void*)DM_get_((DynamicInsertOnlyIntMap__Base*)(dm), (key)))

#define DM_count(dm)                                                          \
    (DM_ASSERT_PTR_COMPAT(dm),                                               \
    DM_count_((DynamicInsertOnlyIntMap__Base*)(dm)))

#define DM_get_value(dm, index)                                               \
    (DM_ASSERT_PTR_COMPAT(dm),                                               \
    (void*)DM_get_value_((DynamicInsertOnlyIntMap__Base*)(dm), index))

#endif //APEXPREDATOR_DYNAMIC_INSERT_ONLY_MAP_H
