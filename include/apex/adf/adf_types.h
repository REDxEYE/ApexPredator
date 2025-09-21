// Created by RED on 20.09.2025.

#ifndef APEXPREDATOR_ADF_TYPES_H
#define APEXPREDATOR_ADF_TYPES_H

#include "zstd.h"
#include "apex/adf/sti_shared.h"
#include "utils/dynamic_array.h"
DYNAMIC_ARRAY_STRUCT(STI_uint8, STI_uint8);


typedef struct{
    DynamicArray_STI_uint8 d; // offset: 0, size: 8
    STI_uint32 ResourceContainerSizeUnpacked; // offset: 16, size: 4
} ResourceContainerSizeUnpacked; // size: 24



typedef struct{
    STI_float32 Vec[4]; // offset: 0, size: 4
} WorldAudioVector4; // size: 16

DYNAMIC_ARRAY_STRUCT(WorldAudioVector4, WorldAudioVector4);


typedef struct{
    DynamicArray_WorldAudioVector4 Points; // offset: 0, size: 8
    DynamicArray_WorldAudioVector4 PhysicsStreamPatchType; // offset: 16, size: 8
    STI_uint32 ZoneIndex; // offset: 32, size: 4
    STI_float32 MinBounds[3]; // offset: 36, size: 4
    STI_float32 MaxBounds[3]; // offset: 48, size: 4
    STI_float32 ResourceContainer[3]; // offset: 60, size: 4
} MedianNormal; // size: 72

DYNAMIC_ARRAY_STRUCT(MedianNormal, MedianNormal);

typedef struct{
    DynamicArray_MedianNormal ZoneNormalData; // offset: 0, size: 8
} WorldAudioPatchNormalData; // size: 16

typedef struct{
    DynamicArray_WorldAudioVector4 Points; // offset: 0, size: 8
    STI_uint32 ZoneIndex; // offset: 16, size: 4
    STI_float32 MinBounds[3]; // offset: 20, size: 4
    STI_float32 MaxBounds[3]; // offset: 32, size: 4
} WorldAudioPatchZoneData; // size: 48

DYNAMIC_ARRAY_STRUCT(WorldAudioPatchZoneData, WorldAudioPatchZoneData);

typedef struct{
    DynamicArray_WorldAudioPatchZoneData ZoneData; // offset: 0, size: 8
} WorldAudioPatchData; // size: 16



typedef struct{
    STI_uint32 X; // offset: 0, size: 4
    STI_uint32 Z; // offset: 4, size: 4
    STI_uint32 Size; // offset: 8, size: 4
    STI_uint32 Index; // offset: 12, size: 4
    STI_uint32 Count; // offset: 16, size: 4
    STI_int16 Children[4]; // offset: 20, size: 2
} ArrayAABB; // size: 28

DYNAMIC_ARRAY_STRUCT(ArrayAABB, ArrayAABB);


DYNAMIC_ARRAY_STRUCT(STI_uint32, STI_uint32);

typedef struct{
    STI_int32 PatchX; // offset: 0, size: 4
    STI_int32 PatchZ; // offset: 4, size: 4
    STI_int32 PatchLod; // offset: 8, size: 4
    DynamicArray_STI_uint32 TriangleIndices; // offset: 16, size: 8
    DynamicArray_ArrayAABB TriangleQuadTree; // offset: 32, size: 8
} TerrainPatchInfo; // size: 48

DYNAMIC_ARRAY_STRUCT(TerrainPatchInfo, TerrainPatchInfo);

typedef struct{
    STI_uint8 IsRidge; // offset: 0, size: 1
    STI_uint32 NumInstances; // offset: 4, size: 4
    STI_uint32 NumOnTreeLine; // offset: 8, size: 4
    STI_uint32 NumOnRidges; // offset: 12, size: 4
    STI_uint32 NumCulledBySize; // offset: 16, size: 4
    STI_uint32 NumCulledByForestMesh; // offset: 20, size: 4
    STI_uint32 NumCulledBySeaLevel; // offset: 24, size: 4
    STI_uint32 NumCulledByThinout; // offset: 28, size: 4
} VegetationBillboardLayerStats; // size: 32


DYNAMIC_ARRAY_STRUCT(STI_String, STI_String);


DYNAMIC_ARRAY_STRUCT(STI_uint16, STI_uint16);

typedef struct{
    DynamicArray_STI_uint16 LocationIndices; // offset: 0, size: 8
    DynamicArray_STI_String LocationNames; // offset: 16, size: 8
    VegetationBillboardLayerStats Stats; // offset: 32, size: 4
} VegetationDebugData; // size: 64

typedef struct{
    STI_uint16 X; // offset: 0, size: 2
    STI_uint16 Z; // offset: 2, size: 2
    uint32 Y:24; // offset: 4, size: 4
    uint8 ZoneIndex:6; // offset: 8, size: 1
    uint8 IsPlanted:1; // offset: 100663304, size: 1
    uint8 IsDestroyed:1; // offset: 117440520, size: 1
    STI_uint32 Rotation; // offset: 12, size: 4
    STI_uint8 Color_R; // offset: 16, size: 1
    STI_uint8 Color_G; // offset: 17, size: 1
    STI_uint8 Color_B; // offset: 18, size: 1
    uint32 NameHash; // offset: 20, size: 4
} VegetationSystemInstance; // size: 24

DYNAMIC_ARRAY_STRUCT(VegetationSystemInstance, VegetationSystemInstance);

typedef struct{
    uint32 Name; // offset: 0, size: 4
    STI_float32 BoundingMin[3]; // offset: 4, size: 4
    STI_float32 BoundingMax[3]; // offset: 16, size: 4
    DynamicArray_VegetationSystemInstance Instances; // offset: 32, size: 8
    DynamicArray_STI_uint32 UsedTypes; // offset: 48, size: 8
    VegetationDebugData VegetationDebugData; // offset: 64, size: 8
} InstanceDataLayer; // size: 128

DYNAMIC_ARRAY_STRUCT(InstanceDataLayer, InstanceDataLayer);

typedef struct{
    STI_int32 PatchX; // offset: 0, size: 4
    STI_int32 PatchZ; // offset: 4, size: 4
    STI_int32 PatchLod; // offset: 8, size: 4
    DynamicArray_InstanceDataLayer InstanceDataLayers; // offset: 16, size: 8
    DynamicArray_TerrainPatchInfo TerrainPatchInfo; // offset: 32, size: 8
} InstanceDataPatch; // size: 48

typedef enum{ // size: 4
    E_BLOCKCOMPRESSIONTYPE_NONE = 0,
    E_BLOCKCOMPRESSIONTYPE_BC3 = 1,
    E_BLOCKCOMPRESSIONTYPE_BC7 = 2,
} BlockCompressionType;

typedef enum{ // size: 4
    E_MEMALLOCATOR_CPU = 0,
    E_MEMALLOCATOR_SHARED = 1,
    E_MEMALLOCATOR_GPU = 2,
} MemAllocator;

typedef struct {
    uint32 unk;
    uint32 offset;
    uint32 unk2;
    uint32 size;
    uint32 unk3;
    uint32 unk4;
}CompressedDataInfo;

typedef struct{
    STI_uint32 UncompressedSize; // offset: 0, size: 4
    CompressedDataInfo Data; // offset: 8, size: 8
    DynamicArray_STI_uint8 DecompressedData;
    MemAllocator UncompressAllocator; // offset: 24, size: 4
} CompressedData; // size: 32

typedef struct{
    STI_uint32 Width; // offset: 0, size: 4
    BlockCompressionType BlockCompressionType; // offset: 8, size: 4
    STI_uint32 Height; // offset: 4, size: 4
    uint32 Tiled:1; // offset: 12, size: 4
    uint32 Srgb:1; // offset: 16777228, size: 4
    uint32 unk1;
    uint32 unk2;
    CompressedData Data; // offset: 16, size: 8
} TerrainTexture; // size: 48

typedef struct{
    STI_float32 AABBMin[3]; // offset: 0, size: 4
    STI_float32 AABBMax[3]; // offset: 12, size: 4
    STI_float32 MinW; // offset: 24, size: 4
    STI_float32 MaxW; // offset: 28, size: 4
} TerrainPrimitive; // size: 32

DYNAMIC_ARRAY_STRUCT(TerrainPrimitive, TerrainPrimitive);


typedef struct{
    STI_float32 BoundingBox[6]; // offset: 0, size: 4
    CompressedData Indices; // offset: 24, size: 8
    STI_uint32 IndexTypeSize; // offset: 56, size: 4
    CompressedData Vertices; // offset: 64, size: 8
    CompressedData Vertices2; // offset: 96, size: 8
    CompressedData QuadInfos; // offset: 128, size: 8
    CompressedData TriangleIndices; // offset: 160, size: 8
    CompressedData GroupTriIndices; // offset: 192, size: 8
} TerrainMesh; // size: 224

typedef struct{
    TerrainMesh TerrainMesh; // offset: 0, size: 8
    DynamicArray_TerrainPrimitive TerrainPrimitive; // offset: 224, size: 8
    TerrainTexture TerrainDisplacementTexture; // offset: 240, size: 8
    TerrainTexture TerrainNormalTexture; // offset: 288, size: 8
    TerrainTexture TerrainTriangleMapTexture; // offset: 336, size: 8
    TerrainTexture TerrainMaterialDuplexTexture; // offset: 384, size: 8
    TerrainTexture TerrainColorTexture; // offset: 432, size: 8
    TerrainTexture TerrainQualityTexture; // offset: 480, size: 8
    TerrainTexture TerrainIndirectionTexture; // offset: 528, size: 8
    TerrainTexture TerrainSSDFAtlas; // offset: 576, size: 8
    uint32 DisplacementDownsampled:1; // offset: 624, size: 4
} TerrainPatch; // size: 632

typedef enum{ // size: 4
    STREAM_PATCH_STATIC_POOL = 0,
    STREAM_PATCH_DYNAMIC = 1,
    STREAM_PATCH_USER = 2,
} StreamPatchMemoryType;

typedef struct{
    STI_uint32 Version; // offset: 0, size: 4
    StreamPatchMemoryType MemoryType; // offset: 4, size: 4
    STI_int32 MemorySize; // offset: 8, size: 4
    STI_int32 PatchPositionX; // offset: 12, size: 4
    STI_int32 PatchPositionZ; // offset: 16, size: 4
    STI_int32 PatchLod; // offset: 20, size: 4
} StreamPatchBlockHeader; // size: 24

typedef struct{
    STI_uint32 Version; // offset: 0, size: 4
    STI_int32 Size; // offset: 4, size: 4
    STI_uint32 DynamicMemoryRequirements; // offset: 8, size: 4
    STI_int32 PatchPositionX; // offset: 12, size: 4
    STI_int32 PatchPositionZ; // offset: 16, size: 4
    STI_int32 PatchLod; // offset: 20, size: 4
} StreamPatchFileHeader; // size: 24

bool read_ResourceContainerSizeUnpacked(Buffer* buffer, ResourceContainerSizeUnpacked* out);
bool read_MedianNormal(Buffer* buffer, MedianNormal* out);
bool read_DynamicArray_MedianNormal(Buffer* buffer, DynamicArray_MedianNormal* out);
bool read_WorldAudioPatchNormalData(Buffer* buffer, WorldAudioPatchNormalData* out);
bool read_WorldAudioVector4(Buffer* buffer, WorldAudioVector4* out);
bool read_DynamicArray_WorldAudioVector4(Buffer* buffer, DynamicArray_WorldAudioVector4* out);
bool read_WorldAudioPatchZoneData(Buffer* buffer, WorldAudioPatchZoneData* out);
bool read_DynamicArray_WorldAudioPatchZoneData(Buffer* buffer, DynamicArray_WorldAudioPatchZoneData* out);
bool read_WorldAudioPatchData(Buffer* buffer, WorldAudioPatchData* out);
bool read_ArrayAABB(Buffer* buffer, ArrayAABB* out);
bool read_DynamicArray_ArrayAABB(Buffer* buffer, DynamicArray_ArrayAABB* out);
bool read_TerrainPatchInfo(Buffer* buffer, TerrainPatchInfo* out);
bool read_DynamicArray_TerrainPatchInfo(Buffer* buffer, DynamicArray_TerrainPatchInfo* out);
bool read_VegetationBillboardLayerStats(Buffer* buffer, VegetationBillboardLayerStats* out);
bool read_DynamicArray_STI_String(Buffer* buffer, DynamicArray_STI_String* out);
bool read_DynamicArray_STI_uint16(Buffer* buffer, DynamicArray_STI_uint16* out);
bool read_VegetationDebugData(Buffer* buffer, VegetationDebugData* out);
bool read_DynamicArray_STI_uint32(Buffer* buffer, DynamicArray_STI_uint32* out);
bool read_VegetationSystemInstance(Buffer* buffer, VegetationSystemInstance* out);
bool read_DynamicArray_VegetationSystemInstance(Buffer* buffer, DynamicArray_VegetationSystemInstance* out);
bool read_InstanceDataLayer(Buffer* buffer, InstanceDataLayer* out);
bool read_DynamicArray_InstanceDataLayer(Buffer* buffer, DynamicArray_InstanceDataLayer* out);
bool read_InstanceDataPatch(Buffer* buffer, InstanceDataPatch* out);
bool read_BlockCompressionType(Buffer* buffer, BlockCompressionType* out);
bool read_TerrainTexture(Buffer* buffer, TerrainTexture* out);
bool read_TerrainPrimitive(Buffer* buffer, TerrainPrimitive* out);
bool read_DynamicArray_TerrainPrimitive(Buffer* buffer, DynamicArray_TerrainPrimitive* out);
bool read_MemAllocator(Buffer* buffer, MemAllocator* out);
bool read_DynamicArray_STI_uint8(Buffer* buffer, DynamicArray_STI_uint8* out);
bool read_CompressedData(Buffer* buffer, CompressedData* out);
bool read_TerrainMesh(Buffer* buffer, TerrainMesh* out);
bool read_TerrainPatch(Buffer* buffer, TerrainPatch* out);
bool read_StreamPatchMemoryType(Buffer* buffer, StreamPatchMemoryType* out);
bool read_StreamPatchBlockHeader(Buffer* buffer, StreamPatchBlockHeader* out);
bool read_StreamPatchFileHeader(Buffer* buffer, StreamPatchFileHeader* out);


static bool read_ResourceContainerSizeUnpacked(Buffer* buffer, ResourceContainerSizeUnpacked* out) {
    if (!read_DynamicArray_STI_uint8(buffer, &out->d)) return false;
    if (!read_STI_uint32(buffer, &out->ResourceContainerSizeUnpacked)) return false;
    return true;
}
static bool read_MedianNormal(Buffer* buffer, MedianNormal* out) {
    if (!read_DynamicArray_WorldAudioVector4(buffer, &out->Points)) return false;
    if (!read_DynamicArray_WorldAudioVector4(buffer, &out->PhysicsStreamPatchType)) return false;
    if (!read_STI_uint32(buffer, &out->ZoneIndex)) return false;
    for (uint32 i = 0; i < 3; ++i) {
        if (!read_STI_float32(buffer, &out->MinBounds[i])) return false;
    }
    for (uint32 i = 0; i < 3; ++i) {
        if (!read_STI_float32(buffer, &out->MaxBounds[i])) return false;
    }
    for (uint32 i = 0; i < 3; ++i) {
        if (!read_STI_float32(buffer, &out->ResourceContainer[i])) return false;
    }
    return true;
}
static bool read_DynamicArray_MedianNormal(Buffer* buffer, DynamicArray_MedianNormal* out) {
    uint32 count = 0;
    uint32 offset = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    DA_init(out, MedianNormal, count);
    int64 original_offset = 0;
    if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
    for (uint32 i = 0; i < count; ++i) {
        if (!read_MedianNormal(buffer, &out->items[i])) return false;
    }
    if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    return true;
}
static bool read_WorldAudioPatchNormalData(Buffer* buffer, WorldAudioPatchNormalData* out) {
    if (!read_DynamicArray_MedianNormal(buffer, &out->ZoneNormalData)) return false;
    return true;
}
static bool read_WorldAudioVector4(Buffer* buffer, WorldAudioVector4* out) {
    for (uint32 i = 0; i < 4; ++i) {
        if (!read_STI_float32(buffer, &out->Vec[i])) return false;
    }
    return true;
}
static bool read_DynamicArray_WorldAudioVector4(Buffer* buffer, DynamicArray_WorldAudioVector4* out) {
    uint32 count = 0;
    uint32 offset = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    DA_init(out, WorldAudioVector4, count);
    int64 original_offset = 0;
    if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
    for (uint32 i = 0; i < count; ++i) {
        if (!read_WorldAudioVector4(buffer, &out->items[i])) return false;
    }
    if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    return true;
}
static bool read_WorldAudioPatchZoneData(Buffer* buffer, WorldAudioPatchZoneData* out) {
    if (!read_DynamicArray_WorldAudioVector4(buffer, &out->Points)) return false;
    if (!read_STI_uint32(buffer, &out->ZoneIndex)) return false;
    for (uint32 i = 0; i < 3; ++i) {
        if (!read_STI_float32(buffer, &out->MinBounds[i])) return false;
    }
    for (uint32 i = 0; i < 3; ++i) {
        if (!read_STI_float32(buffer, &out->MaxBounds[i])) return false;
    }
    return true;
}
static bool read_DynamicArray_WorldAudioPatchZoneData(Buffer* buffer, DynamicArray_WorldAudioPatchZoneData* out) {
    uint32 count = 0;
    uint32 offset = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    DA_init(out, WorldAudioPatchZoneData, count);
    int64 original_offset = 0;
    if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
    for (uint32 i = 0; i < count; ++i) {
        if (!read_WorldAudioPatchZoneData(buffer, &out->items[i])) return false;
    }
    if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    return true;
}
static bool read_WorldAudioPatchData(Buffer* buffer, WorldAudioPatchData* out) {
    if (!read_DynamicArray_WorldAudioPatchZoneData(buffer, &out->ZoneData)) return false;
    return true;
}
static bool read_ArrayAABB(Buffer* buffer, ArrayAABB* out) {
    if (!read_STI_uint32(buffer, &out->X)) return false;
    if (!read_STI_uint32(buffer, &out->Z)) return false;
    if (!read_STI_uint32(buffer, &out->Size)) return false;
    if (!read_STI_uint32(buffer, &out->Index)) return false;
    if (!read_STI_uint32(buffer, &out->Count)) return false;
    for (uint32 i = 0; i < 4; ++i) {
        if (!read_STI_int16(buffer, &out->Children[i])) return false;
    }
    return true;
}
static bool read_DynamicArray_ArrayAABB(Buffer* buffer, DynamicArray_ArrayAABB* out) {
    uint32 count = 0;
    uint32 offset = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    DA_init(out, ArrayAABB, count);
    int64 original_offset = 0;
    if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
    for (uint32 i = 0; i < count; ++i) {
        if (!read_ArrayAABB(buffer, &out->items[i])) return false;
    }
    if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    return true;
}
static bool read_TerrainPatchInfo(Buffer* buffer, TerrainPatchInfo* out) {
    if (!read_STI_int32(buffer, &out->PatchX)) return false;
    if (!read_STI_int32(buffer, &out->PatchZ)) return false;
    if (!read_STI_int32(buffer, &out->PatchLod)) return false;
    if (!read_DynamicArray_STI_uint32(buffer, &out->TriangleIndices)) return false;
    if (!read_DynamicArray_ArrayAABB(buffer, &out->TriangleQuadTree)) return false;
    return true;
}
static bool read_DynamicArray_TerrainPatchInfo(Buffer* buffer, DynamicArray_TerrainPatchInfo* out) {
    uint32 count = 0;
    uint32 offset = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    DA_init(out, TerrainPatchInfo, count);
    int64 original_offset = 0;
    if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
    for (uint32 i = 0; i < count; ++i) {
        if (!read_TerrainPatchInfo(buffer, &out->items[i])) return false;
    }
    if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    return true;
}
static bool read_VegetationBillboardLayerStats(Buffer* buffer, VegetationBillboardLayerStats* out) {
    if (!read_STI_uint8(buffer, &out->IsRidge)) return false;
    if (!read_STI_uint32(buffer, &out->NumInstances)) return false;
    if (!read_STI_uint32(buffer, &out->NumOnTreeLine)) return false;
    if (!read_STI_uint32(buffer, &out->NumOnRidges)) return false;
    if (!read_STI_uint32(buffer, &out->NumCulledBySize)) return false;
    if (!read_STI_uint32(buffer, &out->NumCulledByForestMesh)) return false;
    if (!read_STI_uint32(buffer, &out->NumCulledBySeaLevel)) return false;
    if (!read_STI_uint32(buffer, &out->NumCulledByThinout)) return false;
    return true;
}
static bool read_DynamicArray_STI_String(Buffer* buffer, DynamicArray_STI_String* out) {
    uint32 count = 0;
    uint32 offset = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    DA_init(out, STI_String, count);
    int64 original_offset = 0;
    if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
    for (uint32 i = 0; i < count; ++i) {
        if (!read_STI_String(buffer, &out->items[i])) return false;
    }
    if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    return true;
}
static bool read_DynamicArray_STI_uint16(Buffer* buffer, DynamicArray_STI_uint16* out) {
    uint32 count = 0;
    uint32 offset = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    DA_init(out, STI_uint16, count);
    int64 original_offset = 0;
    if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
    for (uint32 i = 0; i < count; ++i) {
        if (!read_STI_uint16(buffer, &out->items[i])) return false;
    }
    if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    return true;
}
static bool read_VegetationDebugData(Buffer* buffer, VegetationDebugData* out) {
    if (!read_DynamicArray_STI_uint16(buffer, &out->LocationIndices)) return false;
    if (!read_DynamicArray_STI_String(buffer, &out->LocationNames)) return false;
    if (!read_VegetationBillboardLayerStats(buffer, &out->Stats)) return false;
    return true;
}
static bool read_DynamicArray_STI_uint32(Buffer* buffer, DynamicArray_STI_uint32* out) {
    uint32 count = 0;
    uint32 offset = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    DA_init(out, STI_uint32, count);
    int64 original_offset = 0;
    if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
    for (uint32 i = 0; i < count; ++i) {
        if (!read_STI_uint32(buffer, &out->items[i])) return false;
    }
    if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    return true;
}
static bool read_VegetationSystemInstance(Buffer* buffer, VegetationSystemInstance* out) {
    if (!read_STI_uint16(buffer, &out->X)) return false;
    if (!read_STI_uint16(buffer, &out->Z)) return false;
    uint32 bitfield_value = 0;
    if (!read_STI_uint32(buffer, &bitfield_value)) return false;
    out->Y = (bitfield_value >> 0) & 0xFFFFFF;
    out->ZoneIndex = (bitfield_value >> 24) & 0x3F;
    out->IsPlanted = (bitfield_value >> 30) & 0x1;
    out->IsDestroyed = (bitfield_value >> 31) & 0x1;
    if (!read_STI_uint32(buffer, &out->Rotation)) return false;
    if (!read_STI_uint8(buffer, &out->Color_R)) return false;
    if (!read_STI_uint8(buffer, &out->Color_G)) return false;
    if (!read_STI_uint8(buffer, &out->Color_B)) return false;
    return true;
}
static bool read_DynamicArray_VegetationSystemInstance(Buffer* buffer, DynamicArray_VegetationSystemInstance* out) {
    uint32 count = 0;
    uint32 offset = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    DA_init(out, VegetationSystemInstance, count);
    int64 original_offset = 0;
    if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
    for (uint32 i = 0; i < count; ++i) {
        if (!read_VegetationSystemInstance(buffer, &out->items[i])) return false;
    }
    if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    return true;
}
static bool read_InstanceDataLayer(Buffer* buffer, InstanceDataLayer* out) {
    for (uint32 i = 0; i < 3; ++i) {
        if (!read_STI_float32(buffer, &out->BoundingMin[i])) return false;
    }
    for (uint32 i = 0; i < 3; ++i) {
        if (!read_STI_float32(buffer, &out->BoundingMax[i])) return false;
    }
    if (!read_DynamicArray_VegetationSystemInstance(buffer, &out->Instances)) return false;
    if (!read_DynamicArray_STI_uint32(buffer, &out->UsedTypes)) return false;
    if (!read_VegetationDebugData(buffer, &out->VegetationDebugData)) return false;
    return true;
}
static bool read_DynamicArray_InstanceDataLayer(Buffer* buffer, DynamicArray_InstanceDataLayer* out) {
    uint32 count = 0;
    uint32 offset = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    DA_init(out, InstanceDataLayer, count);
    int64 original_offset = 0;
    if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
    for (uint32 i = 0; i < count; ++i) {
        if (!read_InstanceDataLayer(buffer, &out->items[i])) return false;
    }
    if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    return true;
}
static bool read_InstanceDataPatch(Buffer* buffer, InstanceDataPatch* out) {
    if (!read_STI_int32(buffer, &out->PatchX)) return false;
    if (!read_STI_int32(buffer, &out->PatchZ)) return false;
    if (!read_STI_int32(buffer, &out->PatchLod)) return false;
    if (!read_DynamicArray_InstanceDataLayer(buffer, &out->InstanceDataLayers)) return false;
    if (!read_DynamicArray_TerrainPatchInfo(buffer, &out->TerrainPatchInfo)) return false;
    return true;
}
static bool read_BlockCompressionType(Buffer* buffer, BlockCompressionType* out) {
    uint32 value = 0;
    if (!read_STI_uint32(buffer, &value)) return false;
    *out = (BlockCompressionType)value;
    return true;
}
static bool read_TerrainTexture(Buffer* buffer, TerrainTexture* out) {
    if (!read_STI_uint32(buffer, &out->Width)) return false;
    if (!read_BlockCompressionType(buffer, &out->BlockCompressionType)) return false;
    if (!read_STI_uint32(buffer, &out->Height)) return false;
    uint32 bitfield_value = 0;
    if (!read_STI_uint32(buffer, &bitfield_value)) return false;
    out->Tiled = (bitfield_value >> 0) & 0x1;
    out->Srgb = (bitfield_value >> 1) & 0x1;
    if (!read_STI_uint32(buffer, &out->unk1)) return false;
    if (!read_STI_uint32(buffer, &out->unk2)) return false;
    if (!read_CompressedData(buffer, &out->Data)) return false;
    return true;
}
static bool read_TerrainPrimitive(Buffer* buffer, TerrainPrimitive* out) {
    for (uint32 i = 0; i < 3; ++i) {
        if (!read_STI_float32(buffer, &out->AABBMin[i])) return false;
    }
    for (uint32 i = 0; i < 3; ++i) {
        if (!read_STI_float32(buffer, &out->AABBMax[i])) return false;
    }
    if (!read_STI_float32(buffer, &out->MinW)) return false;
    if (!read_STI_float32(buffer, &out->MaxW)) return false;
    return true;
}
static bool read_DynamicArray_TerrainPrimitive(Buffer* buffer, DynamicArray_TerrainPrimitive* out) {
    uint32 count = 0;
    uint32 offset = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    DA_init(out, TerrainPrimitive, count);
    int64 original_offset = 0;
    if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
    if(buffer->set_position(buffer, offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    for (uint32 i = 0; i < count; ++i) {
        if (!read_TerrainPrimitive(buffer, &out->items[i])) return false;
    }
    if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    return true;
}
static bool read_MemAllocator(Buffer* buffer, MemAllocator* out) {
    uint32 value = 0;
    if (!read_STI_uint32(buffer, &value)) return false;
    *out = (MemAllocator)value;
    return true;
}
static bool read_DynamicArray_STI_uint8(Buffer* buffer, DynamicArray_STI_uint8* out) {
    uint32 count = 0;
    uint32 offset = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    DA_init(out, STI_uint8, count);
    int64 original_offset = 0;
    if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
    for (uint32 i = 0; i < count; ++i) {
        if (!read_STI_uint8(buffer, &out->items[i])) return false;
    }
    if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    return true;
}

static bool read_CompressedData(Buffer* buffer, CompressedData* out) {
    if (!read_STI_uint32(buffer, &out->UncompressedSize)) return false;
    if (buffer->read(buffer, &out->Data, sizeof(out->Data),NULL)!=BUFFER_SUCCESS)return false;
    if (!read_MemAllocator(buffer, &out->UncompressAllocator)) return false;

    DA_init(&out->DecompressedData, STI_uint8, out->UncompressedSize);
    out->DecompressedData.count = out->UncompressedSize;
    int64 original_offset = 0;
    if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
    if(buffer->set_position(buffer, out->Data.offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    DynamicArray_STI_uint8 compressed_data = {0};
    DA_init(&compressed_data, STI_uint8, out->Data.size);
    compressed_data.count = out->Data.size;
    if(buffer->read(buffer, compressed_data.items, out->Data.size, NULL)!=BUFFER_SUCCESS) return false;
    if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;

    CompressedHeader* header =(CompressedHeader*)&compressed_data.items[0];
    assert((*(uint32*)header->ident == 'PMOC') && "Invalid compressed data magic");
    if (header->comp_type==zstd) {
        size_t zstd_res = ZSTD_decompress(&out->DecompressedData.items, out->DecompressedData.count, &compressed_data.items[sizeof(CompressedHeader)], out->Data.size-sizeof(CompressedHeader));
        ZSTD_ErrorCode error = ZSTD_isError(zstd_res);
        if (error!=ZSTD_error_no_error) {
            printf("ZSTD decompression error: %s\n", ZSTD_getErrorName(error));
            assert(false && "ZSTD decompression failed");
        }
        assert(zstd_res==header->decomp_size &&  "ZSTD decompression size mismatch");
    }else {
        printf("Unsupported compression type: %d\n", header->comp_type);
        assert(false && "Unsupported compression type");
    }
    DA_free(&compressed_data);
    return true;
}
static bool read_TerrainMesh(Buffer* buffer, TerrainMesh* out) {
    for (uint32 i = 0; i < 6; ++i) {
        if (!read_STI_float32(buffer, &out->BoundingBox[i])) return false;
    }
    if (!read_CompressedData(buffer, &out->Indices)) return false;
    if (!read_STI_uint32(buffer, &out->IndexTypeSize)) return false;
    buffer->set_position(buffer, 4, BUFFER_ORIGIN_CURRENT);
    if (!read_CompressedData(buffer, &out->Vertices)) return false;
    if (!read_CompressedData(buffer, &out->Vertices2)) return false;
    if (!read_CompressedData(buffer, &out->QuadInfos)) return false;
    if (!read_CompressedData(buffer, &out->TriangleIndices)) return false;
    if (!read_CompressedData(buffer, &out->GroupTriIndices)) return false;
    return true;
}
static bool read_TerrainPatch(Buffer* buffer, TerrainPatch* out) {
    if (!read_TerrainMesh(buffer, &out->TerrainMesh)) return false;
    if (!read_DynamicArray_TerrainPrimitive(buffer, &out->TerrainPrimitive)) return false;
    if (!read_TerrainTexture(buffer, &out->TerrainDisplacementTexture)) return false;
    if (!read_TerrainTexture(buffer, &out->TerrainNormalTexture)) return false;
    if (!read_TerrainTexture(buffer, &out->TerrainTriangleMapTexture)) return false;
    if (!read_TerrainTexture(buffer, &out->TerrainMaterialDuplexTexture)) return false;
    if (!read_TerrainTexture(buffer, &out->TerrainColorTexture)) return false;
    if (!read_TerrainTexture(buffer, &out->TerrainQualityTexture)) return false;
    if (!read_TerrainTexture(buffer, &out->TerrainIndirectionTexture)) return false;
    if (!read_TerrainTexture(buffer, &out->TerrainSSDFAtlas)) return false;
    uint8 bitfield_value = 0;
    if (!read_STI_uint8(buffer, &bitfield_value)) return false;
    out->DisplacementDownsampled = (bitfield_value >> 0) & 0x1;
    return true;
}
static bool read_StreamPatchMemoryType(Buffer* buffer, StreamPatchMemoryType* out) {
    uint32 value = 0;
    if (!read_STI_uint32(buffer, &value)) return false;
    *out = (StreamPatchMemoryType)value;
    return true;
}
static bool read_StreamPatchBlockHeader(Buffer* buffer, StreamPatchBlockHeader* out) {
    if (!read_STI_uint32(buffer, &out->Version)) return false;
    if (!read_StreamPatchMemoryType(buffer, &out->MemoryType)) return false;
    if (!read_STI_int32(buffer, &out->MemorySize)) return false;
    if (!read_STI_int32(buffer, &out->PatchPositionX)) return false;
    if (!read_STI_int32(buffer, &out->PatchPositionZ)) return false;
    if (!read_STI_int32(buffer, &out->PatchLod)) return false;
    return true;
}
static bool read_StreamPatchFileHeader(Buffer* buffer, StreamPatchFileHeader* out) {
    if (!read_STI_uint32(buffer, &out->Version)) return false;
    if (!read_STI_int32(buffer, &out->Size)) return false;
    if (!read_STI_uint32(buffer, &out->DynamicMemoryRequirements)) return false;
    if (!read_STI_int32(buffer, &out->PatchPositionX)) return false;
    if (!read_STI_int32(buffer, &out->PatchPositionZ)) return false;
    if (!read_STI_int32(buffer, &out->PatchLod)) return false;
    return true;
}
static void STI_ADF_register_functions(STI_TypeLibrary* lib) {
    void** slot;
    slot = DM_insert(&lib->read_functions, 0x0D811C0C);     *slot = read_TerrainTexture;
    slot = DM_insert(&lib->read_functions, 0x165A5795);     *slot = read_DynamicArray_WorldAudioPatchZoneData;
    slot = DM_insert(&lib->read_functions, 0x1F783B17);     *slot = read_VegetationSystemInstance;
    slot = DM_insert(&lib->read_functions, 0x200D0BC9);     *slot = read_ArrayAABB;
    slot = DM_insert(&lib->read_functions, 0x21A16FA4);     *slot = read_WorldAudioVector4;
    slot = DM_insert(&lib->read_functions, 0x2EEB55AC);     *slot = read_MedianNormal;
    slot = DM_insert(&lib->read_functions, 0x45FEA6F5);     *slot = read_TerrainPrimitive;
    slot = DM_insert(&lib->read_functions, 0x52EEFC8D);     *slot = read_MemAllocator;
    slot = DM_insert(&lib->read_functions, 0x56500CAA);     *slot = read_CompressedData;
    slot = DM_insert(&lib->read_functions, 0x573FA4BF);     *slot = read_DynamicArray_ArrayAABB;
    slot = DM_insert(&lib->read_functions, 0x5A6DE0C2);     *slot = read_StreamPatchMemoryType;
    slot = DM_insert(&lib->read_functions, 0x5EE422D0);     *slot = read_DynamicArray_WorldAudioVector4;
    slot = DM_insert(&lib->read_functions, 0x6032D156);     *slot = read_DynamicArray_TerrainPatchInfo;
    slot = DM_insert(&lib->read_functions, 0x65950F52);     *slot = read_DynamicArray_MedianNormal;
    slot = DM_insert(&lib->read_functions, 0x6D5409F0);     *slot = read_WorldAudioPatchNormalData;
    slot = DM_insert(&lib->read_functions, 0x6F962A30);     *slot = read_DynamicArray_STI_uint8;
    slot = DM_insert(&lib->read_functions, 0x72E3A69A);     *slot = read_DynamicArray_STI_uint16;
    slot = DM_insert(&lib->read_functions, 0x766E412E);     *slot = read_DynamicArray_InstanceDataLayer;
    slot = DM_insert(&lib->read_functions, 0x78CB76FD);     *slot = read_StreamPatchBlockHeader;
    slot = DM_insert(&lib->read_functions, 0x80B0FEC7);     *slot = read_DynamicArray_TerrainPrimitive;
    slot = DM_insert(&lib->read_functions, 0x852D0D20);     *slot = read_StreamPatchFileHeader;
    slot = DM_insert(&lib->read_functions, 0x962828FC);     *slot = read_TerrainMesh;
    slot = DM_insert(&lib->read_functions, 0xA553A921);     *slot = read_VegetationBillboardLayerStats;
    slot = DM_insert(&lib->read_functions, 0xB057437A);     *slot = read_WorldAudioPatchData;
    slot = DM_insert(&lib->read_functions, 0xB9D80AD4);     *slot = read_TerrainPatchInfo;
    slot = DM_insert(&lib->read_functions, 0xC106B357);     *slot = read_DynamicArray_STI_String;
    slot = DM_insert(&lib->read_functions, 0xC49009EE);     *slot = read_ResourceContainerSizeUnpacked;
    slot = DM_insert(&lib->read_functions, 0xC7D58849);     *slot = read_VegetationDebugData;
    slot = DM_insert(&lib->read_functions, 0xD7FF1C92);     *slot = read_WorldAudioPatchZoneData;
    slot = DM_insert(&lib->read_functions, 0xD97AAC30);     *slot = read_BlockCompressionType;
    slot = DM_insert(&lib->read_functions, 0xE8ABFE3B);     *slot = read_DynamicArray_STI_uint32;
    slot = DM_insert(&lib->read_functions, 0xEA0A66F0);     *slot = read_DynamicArray_VegetationSystemInstance;
    slot = DM_insert(&lib->read_functions, 0xF8DD7138);     *slot = read_InstanceDataLayer;
    slot = DM_insert(&lib->read_functions, 0xFD31E1DB);     *slot = read_TerrainPatch;
    slot = DM_insert(&lib->read_functions, 0xFD88832A);     *slot = read_InstanceDataPatch;
}
#endif //APEXPREDATOR_ADF_TYPES_H
