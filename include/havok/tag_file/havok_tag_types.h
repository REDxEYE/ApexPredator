// Created by RED on 12.10.2025.

#ifndef APEXPREDATOR_HAVOK_TYPES_H
#define APEXPREDATOR_HAVOK_TYPES_H

#include <assert.h>
#include "int_def.h"
#include "utils/string.h"
#include "utils/dynamic_array.h"


typedef enum {
    Format = 0x01,
    SubType = 0x02,
    Version = 0x04,
    SizeAlign = 0x08,
    Flags = 0x10,
    Fields = 0x20,
    Interfaces = 0x40,
    Attribute = 0x80
} HKTagFlags;

typedef struct HKType HKTagType;

typedef struct {
    String name;
    uint32 flags;
    uint32 offset;
    HKTagType *type;
} HKTagTypeMember;

DYNAMIC_ARRAY_STRUCT(HKTagTypeMember, HKMember);

void HKTagTypeMember_init(HKTagTypeMember *member, const String *name);

void HKTagTypeMember_free(HKTagTypeMember *member);


typedef struct {
    String name;
    HKTagType *type;
    uint32 number: 30;
    uint32 is_number:1;
    uint32 is_class: 1;
} HKTagTemplateArgument;

typedef struct {
    HKTagType *type;
    uint32 offset;
} HKTagInterface;

DYNAMIC_ARRAY_STRUCT(HKTagTemplateArgument, HKTagTemplateArgument);

DYNAMIC_ARRAY_STRUCT(HKTagInterface, HKInterface);

void HKTagTemplateArgument_init(HKTagTemplateArgument *arg, const String *name);

void HKTagTemplateArgument_free(HKTagTemplateArgument *arg);

typedef enum {
    HKTYPE_PRIMITIVE = 0,
    HKTYPE_UNK1 = 1,
    HKTYPE_UNK2 = 2,
    HKTYPE_STRING = 3,
    HKTYPE_UNK4 = 4,
    HKTYPE_UNK5 = 5,
    HKTYPE_POINTER = 6,
    HKTYPE_RECORD = 7,
    HKTYPE_ARRAY = 8,
} HKTagDataType;

static const char *HKTAGTYPE_NAMES[] = {
    "PRIMITIVE", "UNK1", "UNK2", "STRING", "UNK4", "UNK5", "POINTER", "RECORD", "ARRAY"
};


typedef struct HKType {
    String name;
    DynamicArray_HKTagTemplateArgument template_args;
    DynamicArray_HKMember members;
    HKTagType *parent;
    DynamicArray_HKInterface interfaces;
    uint32 format;
    uint32 sub_type;
    uint32 version;
    uint32 size;
    uint32 align;
    uint32 flags;
    uint32 hash;
    HKTagDataType data_type;
} HKTagType;

void HKTagType_print(const HKTagType *type, FILE *out, uint32 indent);

DYNAMIC_ARRAY_STRUCT(HKTagType, HKTagType);

void HKTagType_init(HKTagType *type, const String *name);

void HKTagType_free(HKTagType *type);

#endif //APEXPREDATOR_HAVOK_TYPES_H