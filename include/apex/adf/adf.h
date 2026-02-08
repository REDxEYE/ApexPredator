// Created by RED on 19.09.2025.

#ifndef APEXPREDATOR_ADF_H
#define APEXPREDATOR_ADF_H
#include <stdbool.h>

#include "adf_type_info_map.h"
#include "platform/common_arrays.h"

#include "utils/buffer/buffer.h"
#include "utils/dynamic_array.h"
#include "utils/json.h"
#include "utils/buffer/memory_buffer.h"

#define ADF_MAGIC " FDA"

typedef enum {
    ADF_Primitive = 0,
    ADF_Structure = 1,
    ADF_Pointer = 2,
    ADF_Array = 3,
    ADF_InlineArray = 4,
    ADF_StringType = 5,
    ADF_Recursive = 6,
    ADF_Bitfield = 7,
    ADF_Enumeration = 8,
    ADF_StringHash = 9,
    ADF_DeferredType = 10,
    ADF_Force_i32 = 0x7FFFFFFF,
} ADFMetaType;

#pragma pack(push, 1)
typedef struct {
    char ident[4];
    uint32 version;
    uint32 instance_count;
    uint32 instance_offset;

    uint32 typedef_count;
    uint32 typedef_offset;

    uint32 stringhash_count;
    uint32 stringhash_offset;

    uint32 nametable_count;
    uint32 nametable_offset;

    uint32 total_size;

    uint32 m_MetaDataOffset;
    uint32 m_FlagField;
    uint32 m_IncludedLibraries;
    uint64 gap;
} ADFHeader;

typedef struct {
    uint32 name_hash;
    uint32 type_hash;
    uint32 offset;
    uint32 size;
    uint64 name_id;
} ADFInstance;

typedef struct {
    ADFMetaType type;
    uint32 size;
    uint32 alignment;
    uint32 hash;
    uint64 name_id;
    uint16 flags;
    uint16 scalar_type;
    uint32 element_type_hash;
    uint32 element_len;
} ADFTypeDef;

typedef struct {
    uint64 name_id;
    uint32 type_hash;
    uint32 size;
    uint32 offset: 24;
    uint32 bit_offset: 8;
    uint32 default_type;
    uint64 default_value;
} ADFStructMemberInfo;

typedef struct {
    uint64 name_id;
    uint32 value;
} ADFEnumMemberInfo;

#pragma pack(pop)

DYNAMIC_ARRAY_STRUCT(ADFTypeDef, ADFTypeDef);
DYNAMIC_ARRAY_STRUCT(ADFStructMemberInfo, ADFStructMemberInfo);
DYNAMIC_ARRAY_STRUCT(ADFEnumMemberInfo, ADFEnumMemberInfo);

typedef struct {
    DynamicArray_ADFStructMemberInfo members;
} ADFStructTypeData;

typedef struct {
    DynamicArray_ADFEnumMemberInfo members;
} ADFEnumTypeData;

typedef struct {
    uint32 count;
} ADFArrayTypeData;

typedef struct {
    uint32 type_hash;
} ADFDeferredTypeData;

typedef union {
    ADFStructTypeData struct_data;
    ADFEnumTypeData enum_data;
    ADFArrayTypeData array_data;
    ADFDeferredTypeData deferred_data;
} ADFTypeData;

typedef struct {
    ADFTypeDef def;
    ADFTypeData type_data;
} ADFType;

void ADFType_free(ADFType* type);
void ADFType_init(ADFType* type, ADFMetaType meta_type);

DYNAMIC_ARRAY_STRUCT(ADFInstance, ADFInstance);
DYNAMIC_ARRAY_STRUCT(ADFType, ADFType);

typedef struct {
    ADFHeader header;
    String comment;
    DynamicArray_String strings;
    DynamicArray_ADFInstance instances;
    DynamicArray_ADFType types;
} ADF;

bool ADF_from_buffer(ADF *adf, Buffer *buffer);

void ADF_free(ADF *adf);

ADF* ADF_load_builtin_adf(const uint8 *data, int64 size);

ADFInstance *ADF_get_instance(ADF *adf, uint32 instance_id);

void *ADF_read_instance(const ADF *adf, const ADFInstance *instance, const MemoryBuffer *mb, const STITypeInfoMap* type_map);

void ADF_free_instance(const ADFInstance *instance, void *instance_data, const STITypeInfoMap* type_map);

void ADF_print_instance(const ADFInstance *instance, const void *instance_data,  JsonContext* ctx, const STITypeInfoMap* type_map);

#endif //APEXPREDATOR_ADF_H
