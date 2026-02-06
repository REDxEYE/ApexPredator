// Created by RED on 06.02.2026.

#ifndef APEXPREDATOR_HAVOK_SUPPORT_TYPES_FUNCTIONS_H
#define APEXPREDATOR_HAVOK_SUPPORT_TYPES_FUNCTIONS_H

#include "havok/havok_support_types.h"
#include "havok/havok_helpers.h"

#include "havok/generated/havok_generated.h"
#include "platform/logger.h"
#include "utils/hash_helper.h"

static void hkVector4f_read(hkVector4f *obj, const TagFile *tf, const uint8 *src) {
    memcpy(obj, src, sizeof(hkVector4f));
}

static void hkVector4f_print(const hkVector4f *obj, JsonContext *ctx) {
    jsonBeginCompactArray(ctx);
    jsonValueFlt(ctx, obj->x);
    jsonValueFlt(ctx, obj->y);
    jsonValueFlt(ctx, obj->z);
    jsonValueFlt(ctx, obj->w);
    jsonEndCompactArray(ctx);
}

static void char_read(char *obj, const TagFile *tf, const uint8 *src) {
    memcpy(obj, src, sizeof(char));
}

static void char_print(const char *obj, JsonContext *ctx) {
    jsonValueStr(ctx, obj);
}

static void signed_char_read(char *obj, const TagFile *tf, const uint8 *src) {
    memcpy(obj, src, sizeof(char));
}

static void signed_char_print(const signed_char *obj, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

static void unsigned_char_read(uint8 *obj, const TagFile *tf, const uint8 *src) {
    memcpy(obj, src, sizeof(uint8));
}

static void unsigned_char_print(const unsigned_char *obj, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

static void int_read(int32 *obj, const TagFile *tf, const uint8 *src) {
    memcpy(obj, src, sizeof(int32));
}

static void int_print(const int32 *obj, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

static void unsigned_int_read(uint32 *obj, const TagFile *tf, const uint8 *src) {
    memcpy(obj, src, sizeof(uint32));
}

static void unsigned_int_print(const uint32 *obj, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

static void short_read(int16 *obj, const TagFile *tf, const uint8 *src) {
    memcpy(obj, src, sizeof(int16));
}

static void short_print(const int16 *obj, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

static void unsigned_short_read(uint16 *obj, const TagFile *tf, const uint8 *src) {
    memcpy(obj, src, sizeof(uint16));
}

static void unsigned_short_print(const uint16 *obj, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

static void uint64_read(uint64 *obj, const TagFile *tf, const uint8 *src) {
    memcpy(obj, src, sizeof(uint64));
}

static void uint64_print(const uint64 *obj, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

static void unsigned_long_long_read(uint64 *obj, const TagFile *tf, const uint8 *src) {
    memcpy(obj, src, sizeof(uint64));
}

static void unsigned_long_long_print(const uint64 *obj, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

static void float_read(float *obj, const TagFile *tf, const uint8 *src) {
    memcpy(obj, src, sizeof(float));
}

static void float_print(const float *obj, JsonContext *ctx) {
    jsonValueFlt(ctx, *obj);
}

static void hkMatrix3Impl_float_read(float *obj, const TagFile *tf, const uint8 *src) {
    memcpy(obj, src, sizeof(float) * 12);
}

static void hkReflect__Detail__Opaque_print(const hkReflect__Detail__Opaque *obj, JsonContext *ctx) {

}

static void hkReflect__Detail__Opaque_read(const TagFile *tf, hkReflect__Detail__Opaque *obj, const uint8 *src) {
}

static void hkReflect__Type_print(const hkReflect__Type *obj, JsonContext *ctx) {
}

static void hkReflect__Type_read(hkReflect__Type *obj, const TagFile *tf, const uint8 *src) {
}

static void hkMatrix3Impl_float_print(const float *obj, JsonContext *ctx) {
    jsonBeginCompactArray(ctx);
    for (int i = 0; i < 12; ++i) {
        jsonValueFlt(ctx, obj[i]);
    }
    jsonEndCompactArray(ctx);
}

static void void_print(const void *obj, JsonContext *ctx) {
    (void) obj;
    printf("Not implemented!");
    exit(1);
}

static void void_read(void *obj, const TagFile *tf, const uint8 *src) {
    (void) tf;
    (void) obj;
    (void) src;
    printf("Not implemented!");
    exit(1);
}

#define IMPLEMENT_EMPTY_STUBS(type_name)\
    static void type_name##_init(const type_name *obj) {\
        (void)(obj);\
    }\
    \
    static void type_name##_free(const type_name *obj) {\
        (void)(obj);\
    }

IMPLEMENT_EMPTY_STUBS(unsigned_int)
IMPLEMENT_EMPTY_STUBS(unsigned_short)
IMPLEMENT_EMPTY_STUBS(int)
IMPLEMENT_EMPTY_STUBS(short)
IMPLEMENT_EMPTY_STUBS(signed_char)
IMPLEMENT_EMPTY_STUBS(unsigned_char)
IMPLEMENT_EMPTY_STUBS(hkVector4f)
IMPLEMENT_EMPTY_STUBS(hkRotationImpl)
IMPLEMENT_EMPTY_STUBS(float)
IMPLEMENT_EMPTY_STUBS(unsigned_long_long)


static void voidPtr_print(const voidPtr *obj, JsonContext *ctx) {
    // HavokControlBlock_print(*obj, ctx);
}


static void voidPtr_read(voidPtr *obj, const TagFile *tf, const uint8 *src) {
    (void) tf;
    (void) obj;
    (void) src;
    printf("Not implemented!");
    exit(1);
}

static void hkRotationImpl_print(const hkRotationImpl *obj, JsonContext *ctx) {
    jsonBeginCompactArray(ctx);
    for (int j = 0; j < 3; ++j) {
        jsonValueFlt(ctx, obj->m_col0[j]);
    }
    for (int j = 0; j < 3; ++j) {
        jsonValueFlt(ctx, obj->m_col1[j]);
    }
    for (int j = 0; j < 3; ++j) {
        jsonValueFlt(ctx, obj->m_col2[j]);
    }
    for (int j = 0; j < 3; ++j) {
        jsonValueFlt(ctx, obj->m_col3[j]);
    }
    jsonEndCompactArray(ctx);
}

static void hkRotationImpl_read(hkRotationImpl *obj, const TagFile *tf, const uint8 *src) {
    memcpy(obj, src, sizeof(hkRotationImpl));
}

static void const_charPtr_print(const const_charPtr *obj, JsonContext *ctx) {
    jsonValueStr(ctx, *obj);
}

static void ptr_read(void **dst, const TagFile *tf, const uint8 *src, uint32_t *ptr_count);

static void const_charPtr_read(const_charPtr *obj, const TagFile *tf, const uint8 *src) {
    ptr_read((void **) obj, tf, src, NULL);
}


static void NamedVariant_print(const void *obj, JsonContext *ctx) {
    const NamedVariant *variant = (NamedVariant *) obj;
    const uint32 hash = hash_cstring(variant->className);
    // const HAVOK_ObjectMethods *methods = DM_get(&lib->object_functions, hash);
    // if (methods == NULL) {
    //     GLog_Error("No methods for NamedVariant with name: %s (hash 0x%08X)", variant->name, hash);
    //     exit(1);
    // }
    // jsonName(ctx, "ptr");
    // methods->print(variant->variant, ctx);
}


static void ptr_read(void **dst, const TagFile *tf, const uint8 *src, uint32_t *ptr_count) {
    uint64 index;
    uint64_read(&index, tf, src);
    if (index == 0) {
        *dst = NULL;
        return;
    }
    const HKItem *item = &tf->items.items[index];
    HKTagType *tag_type = &tf->types.items[item->type];
    const uint32 type_hash = HKTagType_hash(tag_type);
    const HavokTypeInfo **type_info_p = DM_get(&HAVOK_TYPES_type_info, type_hash);
    if (type_info_p == NULL) {
        GLog_Error("No type info for type hash 0x%08X", type_hash);
        abort();
    }
    const HavokTypeInfo *type_info = *type_info_p;

    if (type_info->read == NULL) {
        GLog_Error("No read method for type hash 0x%08X", type_hash);
        exit(1);
    }
    char *out = mp_malloc(type_info->size*item->count);
    *dst = out;
    for (int i = 0; i < item->count; ++i) {
        if (type_info->init != NULL) {
            type_info->init(out + (i * type_info->size));
        }
        type_info->read(out + (i * type_info->size), tf, tf->data.items + item->offset + (i * type_info->disk_size));
    }
    if (ptr_count != NULL) {
        *ptr_count = item->count;
    }
}


static void ptr_free(void *obj) {
    if (obj == NULL)return;
    const TypedPtr *ptr = obj;
    if (ptr->type_info_->free!=NULL) {
        ptr->type_info_->free(obj);
    }
    mp_free(obj);
}

static void hkArray_read(void *dst, const TagFile *tf, const uint8 *src) {
    hkArray *array = dst;
    uint32_t count = 0;
    ptr_read((void *) &array->m_data, tf, src + 0, &count);
    unsigned_int_read((uint32_t *) &array->m_size, tf, src + 8);
    unsigned_int_read((uint32_t *) &array->m_capacityAndFlags, tf, src + 12);
    array->m_size = (int32_t) count;
    array->m_capacityAndFlags = (int32_t) count;
}

static void hkArray_print(const void *obj, JsonContext *ctx) {
    const hkArray *array = obj;
    jsonBeginArray(ctx);
    const HavokTypeInfo* inner_type = array->inner_type_info;
    for (int i = 0; i < array->m_size; ++i) {
        if (inner_type->is_record) {
            jsonBeginObject(ctx);
        }else if (inner_type->is_array) {
            jsonBeginArray(ctx);
        }
        array->inner_type_info->print(array->m_data + i * array->inner_type_info->size, ctx);
        if (inner_type->is_record) {
            jsonEndObject(ctx);
        }else if (inner_type->is_array) {
            jsonEndArray(ctx);
        }
    }
    jsonEndArray(ctx);
}

static void hkArray_free(void *obj) {
    hkArray *array = obj;
    if (array->m_data != NULL) {
        if (array->inner_type_info->free != NULL) {
            for (int i = 0; i < array->m_size; ++i) {
                array->inner_type_info->free(array->m_data + i * array->inner_type_info->size);
            }
        }
        mp_free(array->m_data);
        array->m_data = NULL;
    }
    array->m_size = 0;
    array->m_capacityAndFlags = 0;
}


static void hkStringPtr_read(hkStringPtr *obj, const TagFile *tf, const uint8 *src) {
    ptr_read((void **) &obj->m_data, tf, src, NULL);
}

static void hkStringPtr_print(const hkStringPtr *obj, JsonContext *ctx) {
    if (obj->m_data != NULL) {
        jsonValueStr(ctx, obj->m_data);
    }
    else {
        jsonValueNull(ctx);
    }
}

static void hkStringPtr_free(hkStringPtr *obj) {
    if (obj->m_data != NULL) {
        mp_free((void*)obj->m_data);
        obj->m_data = NULL;
    }
}

#endif // APEXPREDATOR_HAVOK_SUPPORT_TYPES_FUNCTIONS_H