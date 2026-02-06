// Created by RED on 19.09.2025.

#ifndef APEXPREDATOR_STI_H
#define APEXPREDATOR_STI_H
#include <stdio.h>

#include "adf.h"
#include "int_def.h"
#include "sti_shared.h"
#include "utils/string.h"
#include "utils/dynamic_array.h"
#include "utils/dynamic_map.h"
#include "utils/buffer/buffer.h"

typedef enum STI_DataType{
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
    STI_Alias
}STI_DataType;

typedef struct {
    String name;
    uint32 type_hash;
    uint32 size;
    uint32 offset:24;
    uint32 bit_offset:8;
    uint32 default_type;
    uint64 default_value;
} STI_StructMember;

typedef struct {
    String name;
    uint32 value;
} STI_EnumMember;

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
    uint32 type_hash;
} STI_ArrayTypeData;

typedef struct {
    uint32 type_hash;
} DeferredTypeData;


typedef union {
    STI_StructTypeData struct_data;
    STI_EnumTypeData enum_data;
    STI_ArrayTypeData array_data;
    DeferredTypeData deferred_data;
    uint32 bits_data;
} STI_TypeData;


typedef struct STI_Type {
    String name;
    uint32 hash; // Original hash
    uint32 size;
    uint32 alignment;
    // uint32 parent_hash;
    STI_DataType type;
    STI_TypeData data;
}STI_Type;

void STI_Type_init(STI_Type *type, STI_DataType meta_type, uint32 hash, const String* name);
void STI_Type_free(STI_Type *type);

DYNAMIC_ARRAY_STRUCT(STI_Type, STI_Type);
DYNAMIC_ARRAY_STRUCT(STI_Type*, STI_TypePtr);
DYNAMIC_ARRAY_STRUCT(uint32, STI_exportedHashes);
DYNAMIC_ARRAY_STRUCT(uint32, TypeHash);
DYNAMIC_INT_MAP_STRUCT(STI_Type, STI_Type);
DYNAMIC_INT_MAP_STRUCT(TypeHash, TypeHash);

typedef bool (*read_type_fn)(Buffer* buffer, void* out);

// DYNAMIC_ARRAY_STRUCT(STI_ObjectMethods, STI_ObjectMethods);
DYNAMIC_ARRAY_STRUCT(String, HashString);

// DYNAMIC_INT_MAP_STRUCT(STI_ObjectMethods, STI_ObjectMethods);
DYNAMIC_INT_MAP_STRUCT(HashString, HashString);

typedef DynamicIntMap_STI_Type STI_TypeDict;
typedef DynamicIntMap_TypeHash STI_NameHasToTypeHash;
// typedef DynamicIntMap_STI_ObjectMethods STI_FunctionDict;

typedef struct STI_TypeLibrary{
    STI_TypeDict types;
    STI_NameHasToTypeHash already_seen_name_hashes;
    DynamicArray_STI_exportedHashes exported_hashes;
    // STI_FunctionDict object_functions;
} STI_TypeLibrary;

void STI_TypeLibrary_init(STI_TypeLibrary *lib);
STI_Type *STI_TypeLibrary_register_adf_type(STI_TypeLibrary *lib, const ADF* adf, const ADFType* adf_type);
// STI_Type *STI_TypeLibrary_new_type(STI_TypeLibrary *lib, STI_MetaType meta_type, uint32 type_hash, String* name);
STI_Type* STI_TypeLibrary_register_type(STI_TypeLibrary* lib, ADFType* adf_type);
int32 STI_TypeLibrary_types_count(const STI_TypeLibrary *lib);
void STI_TypeLibrary_free(STI_TypeLibrary *lib);
void STI_TypeLibrary_generate_types(STI_TypeLibrary* lib, const String* namespace, FILE *header_output, const String* relative_header_path, FILE* impl_output);

const STI_Type *STI_TypeLibrary_get_type(const STI_TypeLibrary *lib, uint32 type_hash);

void STI_start_type_dump(STI_TypeLibrary* lib);


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
#define STI_TYPE_HASH_DEFERRED  0xDEFE88ED

#endif //APEXPREDATOR_STI_H
