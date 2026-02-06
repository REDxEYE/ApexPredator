// Created by RED on 20.01.2026.

#ifndef APEXPREDATOR_HAVOK_TYPES_H
#define APEXPREDATOR_HAVOK_TYPES_H

#include "havok/havok_type_info_map.h"
#include "utils/dynamic_map.h"


String *Havok_full_tag_type_name(const HKTagType *type);

typedef struct HavokType HavokType;

typedef struct {
    String name;
    uint32 type_hash;
    uint32 flags;
    uint32 offset;
} HavokRecordMember;

void HavokRecordMember_free(HavokRecordMember *member);

DYNAMIC_ARRAY_STRUCT(HavokRecordMember, HavokRecordMember);

typedef enum {
    HK_PRIMITIVE,
    HK_RECORD,
    HK_FIXED_ARRAY,
    HK_ARRAY,
    HK_PTR,
    HK_STRING,
    HK_ENUM,
    HK_BASIC,
    HK_TYPE_COUNT
} HavokTypeMetaType;

static const char *HavokTypeMetaTypeNames[HK_TYPE_COUNT] = {
    "Primitive",
    "Record",
    "FixedArray",
    "Array",
    "Pointer",
    "String",
    "Enum",
    "Basic"
};


typedef struct HavokType{
    String name;
    uint32 hash;
    uint32 parent_hash;
    uint32 size;
    uint32 align;
    uint32 array_size; // Fixed array size
    uint32 inner_type_hash;
    DynamicArray_HavokRecordMember members;
    HavokTypeMetaType type;
} HavokType;

HavokType *HavokType_init(HavokType *type);

uint32 HavokType_hash(const HavokType *type);

void HavokType_free(HavokType *type);

DYNAMIC_ARRAY_STRUCT(HavokType, HavokType);

DYNAMIC_INT_MAP_STRUCT(HavokType, HavokType);

typedef struct Havok_TypeLibrary Havok_TypeLibrary;

struct Havok_TypeLibrary {
    DynamicIntMap_HavokType types;
    DynamicArray_uint64 exported_hashes;
};

void Havok_TypeLibrary_init(Havok_TypeLibrary *lib);

void Havok_TypeLibrary_free(Havok_TypeLibrary *lib);

HavokType *Havok_TypeLibrary_find_by_name(const Havok_TypeLibrary *lib, const char *name);

void Havok_TypeLibrary_copy_from_tag_file(Havok_TypeLibrary *lib, TagFile *tf);

#endif //APEXPREDATOR_HAVOK_TYPES_H
