// Created by RED on 18.09.2025.

#ifndef APEXPREDATOR_DYNAMIC_ARRAY_H
#define APEXPREDATOR_DYNAMIC_ARRAY_H
#include <stdbool.h>

#include "int_def.h"

typedef struct DynamicArray {
    void **items;
    uint32 item_size;
    uint32 count;
    uint32 capacity;
} DynamicArray__Base;

enum {
    DA_GROW_MULT = 2,
};

#define DYNAMIC_ARRAY_STRUCT(element_type, name)\
    typedef struct DynamicArray_##name{\
        element_type* items;\
        uint32 item_size;\
        uint32 count;\
        uint32 capacity;\
    }DynamicArray_##name

typedef bool (*DA_compare_fn)(const void* a, const void* b);

void DA_init_(DynamicArray__Base *da, uint32 item_size, uint32 initial_capacity);
void DA_append_(DynamicArray__Base *da, void *element);
void* DA_append_get_(DynamicArray__Base *da);
void DA_resize_(DynamicArray__Base *da, uint32 new_size);
void *DA_at_(DynamicArray__Base *da, uint32 index);
void DA_free_(DynamicArray__Base *da);
bool DA_contains_(DynamicArray__Base* da, void* element, DA_compare_fn compare_fn);

#define DA_init(da, item_type, initial_capacity) DA_init_((DynamicArray__Base*)da, sizeof(item_type), initial_capacity)
#define DA_append(da, element) DA_append_((DynamicArray__Base*)da, element)
#define DA_append_get(da) DA_append_get_((DynamicArray__Base*)da)
#define DA_resize(da, new_size) DA_resize_((DynamicArray__Base*)da, new_size)
#define DA_at(da, index) ((void*)DA_at_((DynamicArray__Base*)da, index))
#define DA_free(da) DA_free_((DynamicArray__Base*)da)
#define DA_contains(da, element, compare_fn) DA_contains_((DynamicArray__Base*)da, element, (DA_compare_fn)compare_fn)


#endif //APEXPREDATOR_DYNAMIC_ARRAY_H
