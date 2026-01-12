// Created by RED on 04.01.2026.
#include "havok/havok_support_types.h"

void HavokVector4_read(const TagFile *tf, const Havok_TypeLibrary *lib, HavokVector4 *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(HavokVector4));
}

void HavokVector4_print(const HavokVector4 *obj, const Havok_TypeLibrary *lib, JsonContext *ctx) {
    jsonBeginCompactArray(ctx);
    jsonValueFlt(ctx, obj->x);
    jsonValueFlt(ctx, obj->y);
    jsonValueFlt(ctx, obj->z);
    jsonValueFlt(ctx, obj->w);
    jsonEndCompactArray(ctx);
}

void char_read(const TagFile *tf, const Havok_TypeLibrary *lib, char *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(char));
}

void char_print(const char *obj, const Havok_TypeLibrary *lib, JsonContext *ctx) {
    jsonValueStr(ctx, obj);
}

void signed_char_read(const TagFile *tf, const Havok_TypeLibrary *lib, char *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(char));
}

void signed_char_print(const signed_char *obj, const Havok_TypeLibrary *lib, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

void unsigned_char_read(const TagFile *tf, const Havok_TypeLibrary *lib, uint8 *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(uint8));
}

void unsigned_char_print(const unsigned_char *obj, const Havok_TypeLibrary *lib, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

void int_read(const TagFile *tf, const Havok_TypeLibrary *lib, int32 *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(int32));
}

void int_print(const int32 *obj, const Havok_TypeLibrary *lib, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

void unsigned_int_read(const TagFile *tf, const Havok_TypeLibrary *lib, uint32 *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(uint32));
}

void unsigned_int_print(const uint32 *obj, const Havok_TypeLibrary *lib, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

void short_read(const TagFile *tf, const Havok_TypeLibrary *lib, int16 *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(int16));
}

void short_print(const int16 *obj, const Havok_TypeLibrary *lib, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

void unsigned_short_read(const TagFile *tf, const Havok_TypeLibrary *lib, uint16 *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(uint16));
}

void unsigned_short_print(const uint16 *obj, const Havok_TypeLibrary *lib, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

void uint64_read(const TagFile *tf, const Havok_TypeLibrary *lib, uint64 *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(uint64));
}

void uint64_print(const uint64 *obj, const Havok_TypeLibrary *lib, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

void unsigned_long_long_read(const TagFile *tf, const Havok_TypeLibrary *lib, uint64 *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(uint64));
}

void unsigned_long_long_print(const uint64 *obj, const Havok_TypeLibrary *lib, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

void float_read(const TagFile *tf, const Havok_TypeLibrary *lib, float *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(float));
}

void float_print(const float *obj, const Havok_TypeLibrary *lib, JsonContext *ctx) {
    jsonValueFlt(ctx, *obj);
}

void hkMatrix3Impl_float_read(const TagFile *tf, const Havok_TypeLibrary *lib, float *obj, const uint8 *src) {
    memcpy(obj, src, sizeof(float) * 12);
}

void hkMatrix3Impl_float_print(const float *obj, const Havok_TypeLibrary *lib, JsonContext *ctx) {
    jsonBeginCompactArray(ctx);
    for (int i = 0; i < 12; ++i) {
        jsonValueFlt(ctx, obj[i]);
    }
    jsonEndCompactArray(ctx);
}

void hkReflect__Detail__Opaque_read(const TagFile *tf, const Havok_TypeLibrary *lib, hkReflect__Detail__Opaque*obj,
    const uint8 *src) {
    (void) tf;
    (void) obj;
    (void) src;
    printf("Not implemented!");
    exit(1);
}

void hkReflect__Detail__Opaque_print(const hkReflect__Detail__Opaque *obj, const Havok_TypeLibrary *lib, JsonContext *ctx) {
    (void) obj;
    printf("Not implemented!");
    exit(1);
}

void hkReflect__Type_read(const TagFile *tf, const Havok_TypeLibrary *lib, hkReflect__Type*obj, const uint8 *src) {
    (void) tf;
    (void) obj;
    (void) src;
    printf("Not implemented!");
    exit(1);
}

void hkReflect__Type_print(const hkReflect__Type *obj, const Havok_TypeLibrary *lib, JsonContext *ctx) {
    (void) obj;
    printf("Not implemented!");
    exit(1);
}

void void_print(const void *obj, const Havok_TypeLibrary *lib, JsonContext *ctx) {
    (void) obj;
    printf("Not implemented!");
    exit(1);
}

void NamedVariant_print(const void *obj, const Havok_TypeLibrary *lib, JsonContext *ctx) {
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

void read_ptr(const TagFile *tf, const Havok_TypeLibrary *lib, void **dst, const uint8 *src, uint32_t *ptr_count) {
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

void hkArray_read(const TagFile *tf, const Havok_TypeLibrary *lib, void *dst, const uint8 *src) {
    hkArray *array = (hkArray *) dst;
    uint32_t count = 0;
    read_ptr(tf, lib, (void*)&array->m_data, src + 0, &count);
    unsigned_int_read(tf, lib, (uint32_t *) &array->m_size, src + 8);
    unsigned_int_read(tf, lib, (uint32_t *) &array->m_capacityAndFlags, src + 12);
    array->m_size = (int32_t)count;
    array->m_capacityAndFlags = (int32_t)count;
}

void hkArray_print(const void *obj, const Havok_TypeLibrary *lib, JsonContext *ctx, const char *type_name) {
    hkArray *array = (hkArray *) obj;
    jsonBeginArray(ctx);
    const HavokType* type = HavokTypeLib_find_by_name((Havok_TypeLibrary*)lib, type_name);
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
