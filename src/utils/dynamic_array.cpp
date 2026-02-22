// Created by RED on 18.09.2025.

#include <utils/dynamic_array.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/logger.h"
#include "utils/memory_profiling.h"

#define INIT_ITEM_CHECK  assert(da->item_size!=0 && "Uninitialized dynamic array")

DynamicArray__Base *DA_new_(const uint32 item_size, const uint32 initial_capacity) {
    DynamicArray__Base *da = (DynamicArray__Base*)mp_malloc(sizeof(DynamicArray__Base));
    memset(da, 0, sizeof(DynamicArray__Base));
    da->heap_allocated = 1;
    DA_init_(da, item_size, initial_capacity);
    return da;
}

void DA_init_(DynamicArray__Base *da, const uint32 item_size, const uint32 initial_capacity) {
    if (da->items != NULL) {
        if (da->statically_allocated) {
            GLog_Error("Trying to reinitialize statically allocated dynamic array");
            abort();
        }
        mp_free(da->items);
        da->items = NULL;
    }
    da->capacity = initial_capacity;
    da->count = 0;
    da->item_size = item_size;
    if (initial_capacity > 0) {
        da->items = (void**)mp_calloc(initial_capacity, da->item_size);
    }
    else {
        da->items = NULL;
    }
}

// Does copy element data to internal array
void DA_append_(DynamicArray__Base *da, const void *element) {
    INIT_ITEM_CHECK;
    if (da->count >= da->capacity) {
        DA_reserve_(da, da->count + 1);
    }
    if (!element)return;
    void *slot = DA_at(da, da->count++);
    if (slot) {
        memcpy(slot, element, da->item_size);
    }
}

void *DA_append_get_(DynamicArray__Base *da) {
    INIT_ITEM_CHECK;
    const uint32 index = da->count;
    if (da->count + 1 >= da->capacity) {
        DA_reserve_(da, da->count + 1);
    }
    da->count += 1;
    void *slot = DA_at(da, index);
    return slot;
}

void DA_reserve_(DynamicArray__Base *da, const uint32 needed_capacity) {
    INIT_ITEM_CHECK;
    if (needed_capacity > da->capacity) {
        uint32 new_capacity = da->capacity > 0 ? da->capacity : 1;
        while (new_capacity < needed_capacity) {
            new_capacity *= DA_GROW_MULT;
        }
        void *new_items = mp_realloc(da->items, new_capacity * da->item_size);
        if (!new_items) {
            abort();
        }
        memset((char *) new_items + da->count * da->item_size, 0,
               (new_capacity - da->count) * da->item_size);
        da->items = (void**)new_items;
        da->capacity = new_capacity;
    }
}

void *DA_at_(const DynamicArray__Base *da, const uint32 index) {
    INIT_ITEM_CHECK;
    if (index >= da->count) {
        return NULL;
    }
    char *raw_ptr = (char *) da->items;
    return &raw_ptr[da->item_size * index];
}

void DA_free_(DynamicArray__Base *da) {
    if (da->items != NULL && !da->statically_allocated)
        mp_free(da->items);
    da->items = 0;
    da->count = 0;
    da->capacity = 0;
    da->item_size = 0;
    if (da->heap_allocated) {
        mp_free(da);
    }
}

bool DA_contains_(DynamicArray__Base *da, const void *element, const DA_equal_fn equal_fn) {
    INIT_ITEM_CHECK;
    for (int i = 0; i < da->count; ++i) {
        const void *item = DA_at(da, i);
        if (equal_fn(item, element)) {
            return true;
        }
    }
    return false;
}

void *DA_detach_buffer_(DynamicArray__Base *da) {
    INIT_ITEM_CHECK;
    void *items = da->items;
    da->items = NULL;
    da->count = 0;
    da->capacity = 0;
    da->item_size = 0;
    return items;
}

void *DA_get_buffer_(const DynamicArray__Base *da) {
    INIT_ITEM_CHECK;
    return da->items;
}
