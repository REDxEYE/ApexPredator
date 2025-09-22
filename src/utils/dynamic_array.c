// Created by RED on 18.09.2025.

#include "../../include/utils/dynamic_array.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define NULL_ITEM_CHECK  assert(da->items!=NULL && "Uninitialized dynamic array")

void DA_grow(DynamicArray__Base *da);

void DA_init_(DynamicArray__Base *da, uint32 item_size, uint32 initial_capacity) {
    da->capacity = initial_capacity;
    da->count = 0;
    da->item_size = item_size;
    if (initial_capacity > 0) {
        da->items = malloc(initial_capacity * da->item_size);
        memset(da->items, 0, initial_capacity * da->item_size);
    } else {
        da->items = NULL;
    }
}

// Does copy element data to internal array
void DA_append_(DynamicArray__Base *da, void *element) {
    NULL_ITEM_CHECK;
    if (da->count >= da->capacity) {
        DA_grow(da);
    }
    if (!element)return;
    void *slot = DA_at(da, da->count++);
    if (slot) {
        memcpy(slot, element, da->item_size);
    }
}

void *DA_append_get_(DynamicArray__Base *da) {
    NULL_ITEM_CHECK;
    uint32 index = da->count;
    if (da->count + 1 >= da->capacity) {
        DA_grow(da);
    }
    da->count += 1;
    void *slot = DA_at(da, index);
    return slot;
}

void DA_resize_(DynamicArray__Base *da, uint32 new_size) {
    NULL_ITEM_CHECK;
    if (new_size > da->capacity) {
        uint32 new_capacity = da->capacity;
        while (new_capacity < new_size) {
            new_capacity *= DA_GROW_MULT;
        }
        void *new_items = realloc(da->items, new_capacity * da->item_size);
        if (new_items) {
            memset((char *) new_items + da->count * da->item_size, 0,
                   (new_capacity - da->count) * da->item_size);
            da->items = new_items;
            da->capacity = new_capacity;
        }
    }
    if (new_size > da->count) {
        memset((char *) da->items + da->count * da->item_size, 0,
               (new_size - da->count) * da->item_size);
    }
    da->count = new_size;
}

void DA_grow(DynamicArray__Base *da) {
    NULL_ITEM_CHECK;
    uint32 new_capacity = da->capacity * DA_GROW_MULT;
    void *new_items = realloc(da->items, new_capacity * da->item_size);

    if (new_items) {
        memset((char *) new_items + da->count * da->item_size, 0, (new_capacity - da->count) * da->item_size);
        da->items = new_items;
        da->capacity = new_capacity;
    }
}

void *DA_at_(DynamicArray__Base *da, uint32 index) {
    NULL_ITEM_CHECK;
    if (index >= da->count) {
        return NULL;
    }
    char *raw_ptr = (char *) da->items;
    return &raw_ptr[da->item_size * index];
}

void DA_free_(DynamicArray__Base *da) {
    if (da->items == NULL)return;
    free(da->items);
    da->items = 0;
    da->count = 0;
    da->capacity = 0;
    da->item_size = 0;
}

bool DA_contains_(DynamicArray__Base *da, void *element, DA_compare_fn compare_fn) {
    NULL_ITEM_CHECK;
    for (int i = 0; i < da->count; ++i) {
        void *item = DA_at(da, i);
        if (compare_fn(item, element)) {
            return true;
        }
    }
    return false;
}
