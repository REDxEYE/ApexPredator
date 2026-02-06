// Created by RED on 13.10.2025.

#ifndef APEXPREDATOR_HAVOK_HELPERS_H
#define APEXPREDATOR_HAVOK_HELPERS_H
#include "havok/havok_type_info_map.h"
#include "int_def.h"
#include "havok/tag_file/havok_tag_file.h"

typedef struct TypedPtr {
    HavokTypeInfo *type_info_;
} TypedPtr;

TypedPtr* TagFile_get_item(const TagFile *tf, uint32 index);

void TagFile_free_item(TypedPtr *item);

typedef struct hkVector4f {
    float32 x, y, z, w;
} hkVector4f;

typedef struct hkRotationImpl {
    float32 m_col0[3];
    float32 m_col1[3];
    float32 m_col2[3];
    float32 m_col3[3];
} hkRotationImpl;

typedef struct hkStringPtr {
    char *m_data;
} hkStringPtr;

typedef struct NamedVariant {
    const char *name; // offset: 0, flags: 36, size: 8
    const char *className; // offset: 8, flags: 36, size: 8
    void *variant; // offset: 16, flags: 36, size: 8
} NamedVariant;

typedef struct {
    HavokTypeInfo *inner_type_info;
    char *m_data; // offset: 0, flags: 34, size: 8
    int m_size; // offset: 8, flags: 34, size: 4
    int m_capacityAndFlags; // offset: 12, flags: 34, size: 4
} hkArray;

#endif //APEXPREDATOR_HAVOK_HELPERS_H
