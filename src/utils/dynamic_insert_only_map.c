// Created by RED on 19.09.2025.

#include "utils/dynamic_insert_only_map.h"
#include "utils/dynamic_array.h"

#include <assert.h>
#include "int_def.h"
#include <string.h>

void DM_init_(DynamicInsertOnlyIntMap__Base *dm, uint32 item_size, uint32 initial_capacity) {
    DA_init(&dm->keys, uint32, initial_capacity);
    DA_init_(&dm->values, item_size, initial_capacity);
}

// Binary search for exact point, or insertion point
uint32 DM__find_slot(const DynamicInsertOnlyIntMap__Base *dm, uint32 key) {
    uint32 left = 0;
    uint32 right = dm->keys.count;
    while (left < right) {
        uint32 mid = left + (right - left) / 2;
        uint32 mid_key = dm->keys.items[mid];
        if (mid_key == key) {
            return mid; // Key found
        }
        if (mid_key < key) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    return left; // Key not found, return insertion point
}


// Keys are guarantied to be unique. Keys and data - flat sorted array
void *DM__sorted_insert(DynamicInsertOnlyIntMap__Base *dm, uint32 key) {
    uint32 target_slot = DM__find_slot(dm, key);

    if (target_slot >= dm->keys.count) {
        uint32 *new_key = DA_append_get(&dm->keys);
        if (new_key) {
            *new_key = key;
        }
        return DA_append_get(&dm->values);
    }
    // Shift keys
    if (dm->keys.count + 1 >= dm->keys.capacity) {
        DA_resize(&dm->keys, dm->keys.capacity + 1);
    }
    if (dm->values.count + 1 >= dm->values.capacity) {
        DA_resize(&dm->values, dm->values.capacity + 1);
    }
    memmove((char *) dm->keys.items + (target_slot + 1) * sizeof(uint32),
            (char *) dm->keys.items + target_slot * sizeof(uint32),
            (dm->keys.count - target_slot) * sizeof(uint32));
    dm->keys.count += 1;
    uint32 *slot = dm->keys.items + target_slot;
    *slot = key;

    // Shift values
    memmove((char *) dm->values.items + (target_slot + 1) * dm->values.item_size,
            (char *) dm->values.items + target_slot * dm->values.item_size,
            (dm->values.count - target_slot) * dm->values.item_size);
    dm->values.count += 1;
    char *value_slot = ((char *) dm->values.items) + target_slot * dm->values.item_size;
    memset(value_slot, 0, dm->values.item_size);
    return value_slot;
}

void *DM_insert_(DynamicInsertOnlyIntMap__Base *dm, uint32 key) {
    return DM__sorted_insert(dm, key);
}

void *DM_get_(const DynamicInsertOnlyIntMap__Base *dm, uint32 key) {
    uint32 slot = DM__find_slot(dm, key);
    if (slot < dm->keys.count && dm->keys.items[slot] == key) {
        return (char *) dm->values.items + slot * dm->values.item_size;
    }
    return NULL;
}

void * DM_get_value_(const DynamicInsertOnlyIntMap__Base *dm, uint32 index) {
    if (index < dm->values.count) {
        return (char *) dm->values.items + index * dm->values.item_size;
    }
    return NULL;
}

uint32 DM_count_(const DynamicInsertOnlyIntMap__Base *dm) {
    return dm->keys.count;
}
