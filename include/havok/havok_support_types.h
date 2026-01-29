// Created by RED on 13.10.2025.

#ifndef APEXPREDATOR_HAVOK_SUPPORT_TYPES_H
#define APEXPREDATOR_HAVOK_SUPPORT_TYPES_H

#include "havok_codegen.h"
#include "int_def.h"
#include "tag_file/havok_tag_file.h"
#include "utils/json.h"

#define HAS_TYPE(T) (sizeof(T), 1)

typedef struct hkVector4f {
    float32 x, y, z, w;
} hkVector4f;

typedef struct hkRotationImpl {
    float32 m_col0[3];
    float32 m_col1[3];
    float32 m_col2[3];
    float32 m_col3[3];
} hkRotationImpl;

typedef void *voidPtr;
typedef int8 signed_char;
typedef const int8 const_char;
typedef uint32 unsigned_int;
typedef uint16 unsigned_short;
typedef uint8 unsigned_char;
typedef uint64 unsigned_long_long;
typedef const char *const_charPtr;

typedef struct hkReflect__Type {
    HavokTypeInfo *type_info_;
} hkReflect__Type;

typedef struct hkReflect__Detail__Opaque {
    HavokTypeInfo *type_info_;
} hkReflect__Detail__Opaque;

void char_print(const char *obj, JsonContext *ctx);

void char_read(char *obj, const TagFile *tf, const uint8 *src);

void const_charPtr_print(const const_charPtr *obj, JsonContext *ctx);

void const_charPtr_read(const_charPtr *obj, const TagFile *tf, const uint8 *src);

void float_print(const float *obj, JsonContext *ctx);

void float_read(float *obj, const TagFile *tf, const uint8 *src);

void hkMatrix3Impl_float_print(const float *obj, JsonContext *ctx);

void hkMatrix3Impl_float_read(float *obj, const TagFile *tf, const uint8 *src);

void hkReflect__Detail__Opaque_print(const hkReflect__Detail__Opaque *obj, JsonContext *ctx);

void hkReflect__Detail__Opaque_read(const TagFile *tf, hkReflect__Detail__Opaque *obj, const uint8 *src);

void hkReflect__Type_print(const hkReflect__Type *obj, JsonContext *ctx);

void hkReflect__Type_read(hkReflect__Type *obj, const TagFile *tf, const uint8 *src);

void hkRotationImpl_print(const hkRotationImpl *obj, JsonContext *ctx);

void hkRotationImpl_read(hkRotationImpl *obj, const TagFile *tf, const uint8 *src);

void hkVector4f_print(const hkVector4f *obj, JsonContext *ctx);

void hkVector4f_read(hkVector4f *obj, const TagFile *tf, const uint8 *src);

void int_print(const int32 *obj, JsonContext *ctx);

void int_read(int32 *obj, const TagFile *tf, const uint8 *src);

void short_print(const int16 *obj, JsonContext *ctx);

void short_read(int16 *obj, const TagFile *tf, const uint8 *src);

void signed_char_print(const signed_char *obj, JsonContext *ctx);

void signed_char_read(char *obj, const TagFile *tf, const uint8 *src);

void uint64_print(const uint64 *obj, JsonContext *ctx);

void uint64_read(uint64 *obj, const TagFile *tf, const uint8 *src);

void unsigned_char_print(const unsigned_char *obj, JsonContext *ctx);

void unsigned_char_read(uint8 *obj, const TagFile *tf, const uint8 *src);

void unsigned_int_print(const uint32 *obj, JsonContext *ctx);

void unsigned_int_read(uint32 *obj, const TagFile *tf, const uint8 *src);

void unsigned_long_long_print(const uint64 *obj, JsonContext *ctx);

void unsigned_long_long_read(uint64 *obj, const TagFile *tf, const uint8 *src);

void unsigned_short_print(const uint16 *obj, JsonContext *ctx);

void unsigned_short_read(uint16 *obj, const TagFile *tf, const uint8 *src);

void voidPtr_print(const voidPtr *obj, JsonContext *ctx);

void voidPtr_read(voidPtr *obj, const TagFile *tf, const uint8 *src);

void void_print(const void *obj, JsonContext *ctx);

void void_read(void *obj, const TagFile *tf, const uint8 *src);

#define DEFINE_INIT_AND_FREE(type_name)\
    void type_name##_init(const type_name *obj);\
    void type_name##_free(const type_name *obj);

DEFINE_INIT_AND_FREE(unsigned_int)
DEFINE_INIT_AND_FREE(unsigned_short)
DEFINE_INIT_AND_FREE(int)
DEFINE_INIT_AND_FREE(short)
DEFINE_INIT_AND_FREE(signed_char)
DEFINE_INIT_AND_FREE(unsigned_char)
DEFINE_INIT_AND_FREE(hkVector4f)
DEFINE_INIT_AND_FREE(hkRotationImpl)
DEFINE_INIT_AND_FREE(float)
DEFINE_INIT_AND_FREE(unsigned_long_long)


typedef struct hkStringPtr {
    char *m_data;
} hkStringPtr;

void hkStringPtr_init(hkStringPtr *obj);

void hkStringPtr_free(hkStringPtr *obj);

void hkStringPtr_read(hkStringPtr *obj, const TagFile *tf, const uint8 *src);

void hkStringPtr_print(const hkStringPtr *obj, JsonContext *ctx);

typedef struct NamedVariant {
    const char *name; // offset: 0, flags: 36, size: 8
    const char *className; // offset: 8, flags: 36, size: 8
    void *variant; // offset: 16, flags: 36, size: 8
} NamedVariant;

void NamedVariant_print(const void *obj, JsonContext *ctx);

typedef struct {
    HavokTypeInfo *inner_type_info;
    char *m_data; // offset: 0, flags: 34, size: 8
    int m_size; // offset: 8, flags: 34, size: 4
    int m_capacityAndFlags; // offset: 12, flags: 34, size: 4
} hkArray;

void hkArray_read(void *dst, const TagFile *tf, const uint8 *src);

void hkArray_print(const void *obj, JsonContext *ctx);

void hkArray_free(void *obj);

typedef struct TypedPtr {
    HavokTypeInfo *type_info_;
} TypedPtr;

void ptr_read(void **dst, const TagFile *tf, const uint8 *src, uint32_t *ptr_count);

void ptr_free(void *obj);

#endif //APEXPREDATOR_HAVOK_SUPPORT_TYPES_H
