// Created by RED on 13.10.2025.

#ifndef APEXPREDATOR_HAVOK_SUPPORT_TYPES_H
#define APEXPREDATOR_HAVOK_SUPPORT_TYPES_H

#include "havok_codegen.h"
#include "int_def.h"
#include "tag_file/havok_tag_file.h"
#include "utils/hash_helper.h"
#include "utils/json.h"

typedef struct HavokVector4 {
    float32 x, y, z, w;
} HavokVector4;

typedef int8 signed_char;
typedef const int8 const_char;
typedef uint32 unsigned_int;
typedef uint16 unsigned_short;
typedef uint8 unsigned_char;
typedef uint64 unsigned_long_long;

typedef void *hkReflect__Type;
typedef void *hkReflect__Detail__Opaque;

inline void HavokVector4_read(const TagFile *tf, const HavokTypeLib *lib, HavokVector4 *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(HavokVector4));
}

inline void HavokVector4_print(const HavokVector4 *obj, const HavokTypeLib *lib, JsonContext *ctx) {
    jsonBeginCompactArray(ctx);
    jsonValueFlt(ctx, obj->x);
    jsonValueFlt(ctx, obj->y);
    jsonValueFlt(ctx, obj->z);
    jsonValueFlt(ctx, obj->w);
    jsonEndCompactArray(ctx);
}

inline void char_read(const TagFile *tf, const HavokTypeLib *lib, char *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(char));
}

inline void char_print(const char *obj, const HavokTypeLib *lib, JsonContext *ctx) {
    jsonValueStr(ctx, obj);
}

inline void signed_char_read(const TagFile *tf, const HavokTypeLib *lib, char *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(char));
}

inline void signed_char_print(const signed_char *obj, const HavokTypeLib *lib, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

inline void unsigned_char_read(const TagFile *tf, const HavokTypeLib *lib, uint8 *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(uint8));
}

inline void unsigned_char_print(const unsigned_char *obj, const HavokTypeLib *lib, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

inline void int_read(const TagFile *tf, const HavokTypeLib *lib, int32 *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(int32));
}

inline void int_print(const int32 *obj, const HavokTypeLib *lib, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

inline void unsigned_int_read(const TagFile *tf, const HavokTypeLib *lib, uint32 *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(uint32));
}

inline void unsigned_int_print(const uint32 *obj, const HavokTypeLib *lib, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

inline void short_read(const TagFile *tf, const HavokTypeLib *lib, int16 *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(int16));
}

inline void short_print(const int16 *obj, const HavokTypeLib *lib, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

inline void unsigned_short_read(const TagFile *tf, const HavokTypeLib *lib, uint16 *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(uint16));
}

inline void unsigned_short_print(const uint16 *obj, const HavokTypeLib *lib, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

inline void uint64_read(const TagFile *tf, const HavokTypeLib *lib, uint64 *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(uint64));
}

inline void uint64_print(const uint64 *obj, const HavokTypeLib *lib, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

inline void unsigned_long_long_read(const TagFile *tf, const HavokTypeLib *lib, uint64 *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(uint64));
}

inline void unsigned_long_long_print(const uint64 *obj, const HavokTypeLib *lib, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

inline void float_read(const TagFile *tf, const HavokTypeLib *lib, float *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(float));
}

inline void float_print(const float *obj, const HavokTypeLib *lib, JsonContext *ctx) {
    jsonValueFlt(ctx, *obj);
}

inline void hkMatrix3Impl_float_read(const TagFile *tf, const HavokTypeLib *lib, float *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(float) * 12);
}

inline void hkMatrix3Impl_float_print(const float *obj, const HavokTypeLib *lib, JsonContext *ctx) {
    jsonBeginCompactArray(ctx);
    for (int i = 0; i < 12; ++i) {
        jsonValueFlt(ctx, obj[i]);
    }
    jsonEndCompactArray(ctx);
}

inline void hkReflect__Detail__Opaque_read(const TagFile *tf, const HavokTypeLib *lib, hkReflect__Detail__Opaque *obj,
                                           const uint8 *src) {
    (void) tf;
    (void) obj;
    (void) src;
    printf("Not implemented!");
    exit(1);
}

inline void hkReflect__Detail__Opaque_print(const hkReflect__Detail__Opaque *obj, const HavokTypeLib *lib,
                                            JsonContext *ctx) {
    (void) obj;
    printf("Not implemented!");
    exit(1);
}

inline void hkReflect__Type_read(const TagFile *tf, const HavokTypeLib *lib, hkReflect__Type *obj, const uint8 *src) {
    (void) tf;
    (void) obj;
    (void) src;
    printf("Not implemented!");
    exit(1);
}

inline void hkReflect__Type_print(const hkReflect__Type *obj, const HavokTypeLib *lib, JsonContext *ctx) {
    (void) obj;
    printf("Not implemented!");
    exit(1);
}

inline void void_print(const void *obj, const HavokTypeLib *lib, JsonContext *ctx) {
    (void) obj;
    printf("Not implemented!");
    exit(1);
}


typedef void (*readInstanceFn)(const TagFile *tf, const HavokTypeLib *lib, void *obj, const uint8 *src);

struct hkStringPtr;

typedef struct NamedVariant {
    const char *name; // offset: 0, flags: 36, size: 8
    const char *className; // offset: 8, flags: 36, size: 8
    void *variant; // offset: 16, flags: 36, size: 8
} NamedVariant;

inline void NamedVariant_print(const void *obj, const HavokTypeLib *lib, JsonContext *ctx) {
    NamedVariant *variant = (NamedVariant *) obj;
    uint32 hash = hash_cstring(variant->className);
    HAVOK_ObjectMethods *methods = DM_get(&lib->object_functions, hash);
    if (methods == NULL) {
        printf("No methods for NamedVariant with name: %s (hash 0x%08X)\n", variant->name, hash);
        exit(1);
    }
    jsonName(ctx, "ptr");
    methods->print(variant->variant, lib, ctx);
}

typedef struct {
    char *m_data; // offset: 0, flags: 34, size: 8
    int m_size; // offset: 8, flags: 34, size: 4
    int m_capacityAndFlags; // offset: 12, flags: 34, size: 4
} hkArray;

inline void read_ptr(const TagFile *tf, const HavokTypeLib *lib, void **dst, const uint8 *src, uint32_t* ptr_count) {
    uint64 index;
    uint64_read(tf, lib, &index, src);
    if (index == 0) {
        *dst = NULL;
        return;
    }
    const HKItem *item = &tf->items.items[index];
    const HKTagType *tag_type = &tf->types.items[item->type];
    String *full_type_name = Havok_full_tag_type_name(tag_type);
    const uint32 type_hash = hash_string(full_type_name);
    String_free(full_type_name);
    const HavokType *item_type = DM_get(&lib->types, type_hash);
    if (item_type == NULL) {
        printf("No type for type hash 0x%08X\n", type_hash);
        exit(1);
    }
    const HAVOK_ObjectMethods *item_methods = DM_get(&lib->object_functions, type_hash);
    if (item_methods == NULL) {
        printf("No methods for type hash 0x%08X\n", type_hash);
        exit(1);
    }
    uint8 *out = *dst = malloc(item_type->size * item->count);
    for (int i = 0; i < item->count; ++i) {
        item_methods->read(tf, lib, out + (i * item_type->size), tf->data.items + item->offset + (i * item_type->size));
    }
    if (ptr_count!=NULL) {
        *ptr_count = item->count;
    }
}

inline void hkArray_read(const TagFile *tf, const HavokTypeLib *lib, void *dst, const uint8 *src) {
    hkArray *array = (hkArray *) dst;
    uint32_t count = 0;
    read_ptr(tf, lib, (void*)&array->m_data, src + 0, &count);
    unsigned_int_read(tf, lib, (uint32_t *) &array->m_size, src + 8);
    unsigned_int_read(tf, lib, (uint32_t *) &array->m_capacityAndFlags, src + 12);
    array->m_size = (int32_t)count;
    array->m_capacityAndFlags = (int32_t)count;
}

inline void hkArray_print(const void *obj, const HavokTypeLib *lib, JsonContext *ctx, const char* type_name) {
    hkArray *array = (hkArray *) obj;
    jsonBeginArray(ctx);
    const HavokType* type = HavokTypeLib_find_by_name((HavokTypeLib*)lib, type_name);
    const HAVOK_ObjectMethods *methods = DM_get(&lib->object_functions, hash_cstring(type_name));
    if (methods==NULL || methods->print==NULL) {
        printf("No print methods for hkArray of type %s\n", type_name);
        exit(1);
    }
    for (int i = 0; i < array->m_size; ++i) {
        methods->print(array->m_data + type->size*i, lib, ctx);
    }
    jsonEndArray(ctx);
}

#endif //APEXPREDATOR_HAVOK_SUPPORT_TYPES_H
