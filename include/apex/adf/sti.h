// Created by RED on 19.09.2025.

#ifndef APEXPREDATOR_STI_H
#define APEXPREDATOR_STI_H
#include <stdio.h>

#include "int_def.h"
#include "utils/string.h"
#include "utils/dynamic_array.h"
#include "utils/dynamic_insert_only_map.h"
#include "utils/buffer/buffer.h"

typedef enum {
    STI_Primitive = 0,
    STI_Structure = 1,
    STI_Pointer = 2,
    STI_Array = 3,
    STI_InlineArray = 4,
    STI_StringType = 5,
    STI_MetaType6 = 6,
    STI_Bitfield = 7,
    STI_Enumeration = 8,
    STI_StringHash = 9,
    STI_Force_i32 = 0x7FFFFFFF,
} STI_MetaType;

#pragma pack(push, 1)
typedef struct {
    STI_MetaType type;
    uint32 size;
    uint32 alignment;
    uint32 type_hash;
    uint64 name_id;
    uint32 flags;
    uint32 element_type_hash;
    uint32 element_len;
} STI_TypeDef;

typedef struct {
    uint64 name_id;
    uint32 type_hash;
    uint32 size;
    uint32 offset;
    uint32 default_type;
    uint64 default_value;
} STI_StructMemberInfo;

typedef struct {
    uint64 name_id;
    uint32 value;
} STI_EnumMemberInfo;
#pragma pack(pop)

typedef struct {
    String name;
    STI_StructMemberInfo info;
}STI_StructMember;

typedef struct {
    String name;
    STI_EnumMemberInfo info;
} STI_EnumMember;



DYNAMIC_ARRAY_STRUCT(STI_TypeDef, STI_TypeDef);
DYNAMIC_ARRAY_STRUCT(STI_StructMember, STI_StructMember);
DYNAMIC_ARRAY_STRUCT(STI_EnumMember, STI_EnumMember);

typedef struct {
    DynamicArray_STI_StructMember members;
} STI_StructTypeData;

typedef struct {
    DynamicArray_STI_EnumMember members;
} STI_EnumTypeData;

typedef struct {
    uint32 count;
} STI_ArrayTypeData;

typedef struct {
    uint32 unk;
} STI_UnkTypeData;

typedef union {
    STI_StructTypeData struct_data;
    STI_EnumTypeData enum_data;
    STI_ArrayTypeData array_data;
    STI_UnkTypeData unk_data;
} STI_TypeData;

typedef struct {
    STI_MetaType type;
    STI_TypeDef type_info;
    String name;
    STI_TypeData type_data;
} STI_Type;

DYNAMIC_ARRAY_STRUCT(STI_Type, STI_Type);
DYNAMIC_ARRAY_STRUCT(uint32, STI_exportedHashes);

DYNAMIC_INSERT_ONLY_INT_MAP_STRUCT(STI_Type, STI_Type);

typedef bool (*read_type_fn)(Buffer* buffer, void* out);

DYNAMIC_ARRAY_STRUCT(read_type_fn, read_type_fn);

DYNAMIC_INSERT_ONLY_INT_MAP_STRUCT(read_type_fn, read_type_fn);

typedef DynamicInsertOnlyIntMap_STI_Type STI_TypeDict;
typedef DynamicInsertOnlyIntMap_read_type_fn STI_FunctionDict;

typedef struct {
    STI_TypeDict types;
    DynamicArray_STI_exportedHashes exported_hashes;
    STI_FunctionDict read_functions;
} STI_TypeLibrary;

void STI_TypeLibrary_init(STI_TypeLibrary *lib);
STI_Type *STI_TypeLibrary_new_type(STI_TypeLibrary *lib, STI_MetaType type, uint32 type_hash);
uint32 STI_TypeLibrary_types_count(const STI_TypeLibrary* lib);

void STI_start_type_dump(STI_TypeLibrary* lib);
void STI_dump_type(STI_TypeLibrary* lib, STI_Type* type, FILE* output);
void STI_dump_primitives(STI_TypeLibrary* lib, FILE* output);
void STI_generate_reader_function(STI_TypeLibrary* lib, STI_Type* type, FILE* output, bool prototype_only);
void STI_generate_register_function(STI_TypeLibrary* lib, String* namespace, FILE* output);

#endif //APEXPREDATOR_STI_H
