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

void HavokVector4_read(const TagFile *tf, const Havok_TypeLibrary *lib, HavokVector4 *obj, const uint8 *src);

void HavokVector4_print(const HavokVector4 *obj, const Havok_TypeLibrary *lib, JsonContext *ctx);

void char_read(const TagFile *tf, const Havok_TypeLibrary *lib, char *obj, const uint8 *src);

void char_print(const char *obj, const Havok_TypeLibrary *lib, JsonContext *ctx);

void signed_char_read(const TagFile *tf, const Havok_TypeLibrary *lib, char *obj, const uint8 *src);

void signed_char_print(const signed_char *obj, const Havok_TypeLibrary *lib, JsonContext *ctx);

void unsigned_char_read(const TagFile *tf, const Havok_TypeLibrary *lib, uint8 *obj, const uint8 *src);

void unsigned_char_print(const unsigned_char *obj, const Havok_TypeLibrary *lib, JsonContext *ctx);

void int_read(const TagFile *tf, const Havok_TypeLibrary *lib, int32 *obj, const uint8 *src);

void int_print(const int32 *obj, const Havok_TypeLibrary *lib, JsonContext *ctx);

void unsigned_int_read(const TagFile *tf, const Havok_TypeLibrary *lib, uint32 *obj, const uint8 *src);

void unsigned_int_print(const uint32 *obj, const Havok_TypeLibrary *lib, JsonContext *ctx);

void short_read(const TagFile *tf, const Havok_TypeLibrary *lib, int16 *obj, const uint8 *src);

void short_print(const int16 *obj, const Havok_TypeLibrary *lib, JsonContext *ctx);

void unsigned_short_read(const TagFile *tf, const Havok_TypeLibrary *lib, uint16 *obj, const uint8 *src);

void unsigned_short_print(const uint16 *obj, const Havok_TypeLibrary *lib, JsonContext *ctx);

void uint64_read(const TagFile *tf, const Havok_TypeLibrary *lib, uint64 *obj, const uint8 *src);

void uint64_print(const uint64 *obj, const Havok_TypeLibrary *lib, JsonContext *ctx);

void unsigned_long_long_read(const TagFile *tf, const Havok_TypeLibrary *lib, uint64 *obj, const uint8 *src);

void unsigned_long_long_print(const uint64 *obj, const Havok_TypeLibrary *lib, JsonContext *ctx);

void float_read(const TagFile *tf, const Havok_TypeLibrary *lib, float *obj, const uint8 *src);

void float_print(const float *obj, const Havok_TypeLibrary *lib, JsonContext *ctx);

void hkMatrix3Impl_float_read(const TagFile *tf, const Havok_TypeLibrary *lib, float *obj, const uint8 *src);

void hkMatrix3Impl_float_print(const float *obj, const Havok_TypeLibrary *lib, JsonContext *ctx);

void hkReflect__Detail__Opaque_read(const TagFile *tf, const Havok_TypeLibrary *lib, hkReflect__Detail__Opaque *obj,
                                    const uint8 *src);

void hkReflect__Detail__Opaque_print(const hkReflect__Detail__Opaque *obj, const Havok_TypeLibrary *lib,
                                     JsonContext *ctx);

void hkReflect__Type_read(const TagFile *tf, const Havok_TypeLibrary *lib, hkReflect__Type *obj, const uint8 *src);

void hkReflect__Type_print(const hkReflect__Type *obj, const Havok_TypeLibrary *lib, JsonContext *ctx);

void void_print(const void *obj, const Havok_TypeLibrary *lib, JsonContext *ctx);


typedef void (*readInstanceFn)(const TagFile *tf, const Havok_TypeLibrary *lib, void *obj, const uint8 *src);

struct hkStringPtr;

typedef struct NamedVariant {
    const char *name; // offset: 0, flags: 36, size: 8
    const char *className; // offset: 8, flags: 36, size: 8
    void *variant; // offset: 16, flags: 36, size: 8
} NamedVariant;

void NamedVariant_print(const void *obj, const Havok_TypeLibrary *lib, JsonContext *ctx);

typedef struct {
    char *m_data; // offset: 0, flags: 34, size: 8
    int m_size; // offset: 8, flags: 34, size: 4
    int m_capacityAndFlags; // offset: 12, flags: 34, size: 4
} hkArray;

void read_ptr(const TagFile *tf, const Havok_TypeLibrary *lib, void **dst, const uint8 *src, uint32_t* ptr_count);

void hkArray_read(const TagFile *tf, const Havok_TypeLibrary *lib, void *dst, const uint8 *src);

void hkArray_print(const void *obj, const Havok_TypeLibrary *lib, JsonContext *ctx, const char* type_name);

#endif //APEXPREDATOR_HAVOK_SUPPORT_TYPES_H
