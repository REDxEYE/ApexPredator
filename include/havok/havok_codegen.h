// Created by RED on 12.10.2025.

#ifndef APEXPREDATOR_HAVOK_CODEGEN_H
#define APEXPREDATOR_HAVOK_CODEGEN_H

#include "tag_file/havok_tag_file.h"
#include "utils/dynamic_insert_only_map.h"

typedef struct {
    String name;
    uint32 type_hash;

    uint32 number: 30;
    uint32 is_number: 1;
    uint32 is_class: 1;
} HavokTemplateArgument;

void HavokTemplateArgument_free(HavokTemplateArgument *arg);

DYNAMIC_ARRAY_STRUCT(HavokTemplateArgument, HavokTemplateArgument);

typedef struct {
    String name;
    uint32 type_hash;
    uint32 flags;
    uint32 offset;
} HavokRecordMember;

void HavokRecordMember_free(HavokRecordMember *member);

DYNAMIC_ARRAY_STRUCT(HavokRecordMember, HavokRecordMember);

typedef struct {
    String name;
    uint32 hash;
    uint32 parent_hash;
    uint32 size;
    uint32 align;
    DynamicArray_HavokTemplateArgument template_arguments;
    DynamicArray_HavokRecordMember members;
    uint32 is_record: 1;
    uint32 is_array: 1;
    uint32 is_ptr: 1;
    uint32 is_enum: 1;
    uint32 is_primitive:1;
} HavokType;

void HavokType_init(HavokType *type);

void HavokType_free(HavokType *type);

DYNAMIC_ARRAY_STRUCT(HavokType, HavokType);

DYNAMIC_INSERT_ONLY_INT_MAP_STRUCT(HavokType, HavokType);

typedef struct {
    DynamicInsertOnlyIntMap_HavokType types;
    DynamicArray_uint64 exported_hashes;
} HavokTypeLib;

String *HavokTypeLib_full_type_name(const HKTagType *type);

void HavokTypeLib_init(HavokTypeLib *lib);

void HavokTypeLib_free(HavokTypeLib *lib);

void HavokTypeLib_copy_from_tag_file(HavokTypeLib *lib, TagFile *tf);

void HavokTypeLib_generate_code(HavokTypeLib *lib, const String *namespace, FILE *header_output,
                                const String *header_relative_path, FILE *impl_output);

#endif //APEXPREDATOR_HAVOK_CODEGEN_H
