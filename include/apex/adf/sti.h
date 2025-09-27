// Created by RED on 19.09.2025.

#ifndef APEXPREDATOR_STI_H
#define APEXPREDATOR_STI_H
#include <stdio.h>

#include "int_def.h"
#include "sti_shared.h"
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
    STI_Recursive = 6,
    STI_Bitfield = 7,
    STI_Enumeration = 8,
    STI_StringHash = 9,
    STI_DeferredType = 10,
    STI_Force_i32 = 0x7FFFFFFF,
} STI_MetaType;

#pragma pack(push, 1)
typedef struct {
    STI_MetaType type;
    uint32 size;
    uint32 alignment;
    uint32 hash;
    uint64 name_id;
    uint16 flags;
    uint16 scalar_type;
    uint32 element_type_hash;
    uint32 element_len;
} STI_TypeDef;

typedef struct {
    uint64 name_id;
    uint32 type_hash;
    uint32 size;
    uint32 offset:24;
    uint32 bit_offset:8;
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
    STI_TypeDef info;
    String name;
    STI_TypeData type_data;
} STI_Type;

void STI_Type_free(STI_Type* type);
void STI_Type_init(STI_Type* type, STI_MetaType meta_type);

DYNAMIC_ARRAY_STRUCT(STI_Type, STI_Type);
DYNAMIC_ARRAY_STRUCT(STI_Type*, STI_TypePtr);
DYNAMIC_ARRAY_STRUCT(uint32, STI_exportedHashes);
DYNAMIC_ARRAY_STRUCT(uint32, TypeHash);
DYNAMIC_INSERT_ONLY_INT_MAP_STRUCT(STI_Type, STI_Type);
DYNAMIC_INSERT_ONLY_INT_MAP_STRUCT(TypeHash, TypeHash);

typedef bool (*read_type_fn)(Buffer* buffer, void* out);

DYNAMIC_ARRAY_STRUCT(STI_ObjectMethods, STI_ObjectMethods);
DYNAMIC_ARRAY_STRUCT(String, HashString);

DYNAMIC_INSERT_ONLY_INT_MAP_STRUCT(STI_ObjectMethods, STI_ObjectMethods);
DYNAMIC_INSERT_ONLY_INT_MAP_STRUCT(HashString, HashString);

typedef DynamicInsertOnlyIntMap_STI_Type STI_TypeDict;
typedef DynamicInsertOnlyIntMap_TypeHash STI_NameHasToTypeHash;
typedef DynamicInsertOnlyIntMap_STI_ObjectMethods STI_FunctionDict;

typedef struct STI_TypeLibrary{
    STI_TypeDict types;
    STI_NameHasToTypeHash name_hash_to_type;
    DynamicArray_STI_exportedHashes exported_hashes;
    STI_FunctionDict object_functions;

    DynamicInsertOnlyIntMap_HashString hash_strings;
} STI_TypeLibrary;

void STI_TypeLibrary_init(STI_TypeLibrary *lib);
STI_Type *STI_TypeLibrary_new_type(STI_TypeLibrary *lib, STI_MetaType type, uint32 type_hash, String* name);
int32 STI_TypeLibrary_types_count(const STI_TypeLibrary *lib);
void STI_TypeLibrary_free(STI_TypeLibrary *lib);
void STI_TypeLibrary_generate_types(STI_TypeLibrary* lib, String* namespace, FILE *header_output, String* relative_header_path, FILE* impl_output);

void STI_start_type_dump(STI_TypeLibrary* lib);
void STI_dump_type(STI_TypeLibrary* lib, STI_Type* type, FILE* output);
void STI_generate_reader_function(STI_TypeLibrary* lib, STI_Type* type, FILE* output, bool prototype_only);
void STI_generate_free_function(STI_TypeLibrary* lib, STI_Type* type, FILE* output, bool prototype_only);
void STI_generate_print_function(STI_TypeLibrary* lib, STI_Type* type, FILE* output, bool prototype_only);
void STI_generate_register_function(STI_TypeLibrary* lib, String* namespace, FILE* output);


#define STI_TYPE_HASH_INT8  0x580D0A62
#define STI_TYPE_HASH_UINT8  0x0CA2821D
#define STI_TYPE_HASH_INT16  0xD13FCF93
#define STI_TYPE_HASH_UINT16  0x86D152BD
#define STI_TYPE_HASH_INT32  0x192FE633
#define STI_TYPE_HASH_UINT32  0x075E4E4F
#define STI_TYPE_HASH_INT64  0xAF41354F
#define STI_TYPE_HASH_UINT64  0xA139E01F
#define STI_TYPE_HASH_FLOAT32  0x7515A207
#define STI_TYPE_HASH_FLOAT64  0xC609F663
#define STI_TYPE_HASH_STRING  0x8955583E
#define STI_TYPE_HASH_UNK  0xDEFE88ED

#endif //APEXPREDATOR_STI_H
