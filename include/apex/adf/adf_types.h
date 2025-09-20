// Created by RED on 20.09.2025.

#ifndef APEXPREDATOR_ADF_TYPES_H
#define APEXPREDATOR_ADF_TYPES_H

#include "apex/adf/sti_shared.h"
#include "utils/dynamic_array.h"

DYNAMIC_ARRAY_STRUCT(STI_uint8, STI_uint8);


typedef struct{
    DynamicArray_STI_uint8 d;
    STI_uint32 ResourceContainerSizeUnpacked;
} ResourceContainerSizeUnpacked;



typedef struct{
    STI_float32 Vec[4];
} WorldAudioVector4;

DYNAMIC_ARRAY_STRUCT(WorldAudioVector4, WorldAudioVector4);


typedef struct{
    DynamicArray_WorldAudioVector4 Points;
    DynamicArray_WorldAudioVector4 PhysicsStreamPatchType;
    STI_uint32 ZoneIndex;
    STI_float32 MinBounds[3];
    STI_float32 MaxBounds[3];
    STI_float32 ResourceContainer[3];
} MedianNormal;

DYNAMIC_ARRAY_STRUCT(MedianNormal, MedianNormal);

typedef struct{
    DynamicArray_MedianNormal ZoneNormalData;
} WorldAudioPatchNormalData;

typedef struct{
    DynamicArray_WorldAudioVector4 Points;
    STI_uint32 ZoneIndex;
    STI_float32 MinBounds[3];
    STI_float32 MaxBounds[3];
} WorldAudioPatchZoneData;

DYNAMIC_ARRAY_STRUCT(WorldAudioPatchZoneData, WorldAudioPatchZoneData);

typedef struct{
    DynamicArray_WorldAudioPatchZoneData ZoneData;
} WorldAudioPatchData;



typedef struct{
    STI_uint32 X;
    STI_uint32 Z;
    STI_uint32 Size;
    STI_uint32 Index;
    STI_uint32 Count;
    STI_int16 Children[4];
} ArrayAABB;

DYNAMIC_ARRAY_STRUCT(ArrayAABB, ArrayAABB);


DYNAMIC_ARRAY_STRUCT(STI_uint32, STI_uint32);

typedef struct{
    STI_int32 PatchX;
    STI_int32 PatchZ;
    STI_int32 PatchLod;
    DynamicArray_STI_uint32 TriangleIndices;
    DynamicArray_ArrayAABB TriangleQuadTree;
} TerrainPatchInfo;

DYNAMIC_ARRAY_STRUCT(TerrainPatchInfo, TerrainPatchInfo);

typedef struct{
    STI_uint8 IsRidge;
    STI_uint32 NumInstances;
    STI_uint32 NumOnTreeLine;
    STI_uint32 NumOnRidges;
    STI_uint32 NumCulledBySize;
    STI_uint32 NumCulledByForestMesh;
    STI_uint32 NumCulledBySeaLevel;
    STI_uint32 NumCulledByThinout;
} VegetationBillboardLayerStats;


DYNAMIC_ARRAY_STRUCT(STI_String, STI_String);


DYNAMIC_ARRAY_STRUCT(STI_uint16, STI_uint16);

typedef struct{
    DynamicArray_STI_uint16 LocationIndices;
    DynamicArray_STI_String LocationNames;
    VegetationBillboardLayerStats Stats;
} VegetationDebugData;

typedef struct{
    STI_uint16 X;
    STI_uint16 Z;
    uint32 Y:24;
    uint8 ZoneIndex:6;
    uint8 IsPlanted:1;
    uint8 IsDestroyed:1;
    STI_uint32 Rotation;
    STI_uint8 Color_R;
    STI_uint8 Color_G;
    STI_uint8 Color_B;
    uint32 NameHash;
} VegetationSystemInstance;

DYNAMIC_ARRAY_STRUCT(VegetationSystemInstance, VegetationSystemInstance);

typedef struct{
    uint32 Name;
    STI_float32 BoundingMin[3];
    STI_float32 BoundingMax[3];
    DynamicArray_VegetationSystemInstance Instances;
    DynamicArray_STI_uint32 UsedTypes;
    VegetationDebugData VegetationDebugData;
} InstanceDataLayer;

DYNAMIC_ARRAY_STRUCT(InstanceDataLayer, InstanceDataLayer);

typedef struct{
    STI_int32 PatchX;
    STI_int32 PatchZ;
    STI_int32 PatchLod;
    DynamicArray_InstanceDataLayer InstanceDataLayers;
    DynamicArray_TerrainPatchInfo TerrainPatchInfo;
} InstanceDataPatch;

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

typedef struct{
    STI_uint32 UncompressedSize;
    DynamicArray_STI_uint8 Data;
    MemAllocator UncompressAllocator;
} CompressedData;

typedef struct{
    STI_uint32 Width;
    STI_uint32 Height;
    BlockCompressionType BlockCompressionType;
    uint32 Tiled:1;
    uint32 Srgb:1;
    CompressedData Data;
} TerrainTexture;

typedef struct{
    STI_float32 AABBMin[3];
    STI_float32 AABBMax[3];
    STI_float32 MinW;
    STI_float32 MaxW;
} TerrainPrimitive;

DYNAMIC_ARRAY_STRUCT(TerrainPrimitive, TerrainPrimitive);


typedef struct{
    STI_float32 BoundingBox[6];
    CompressedData Indices;
    STI_uint32 IndexTypeSize;
    CompressedData Vertices;
    CompressedData Vertices2;
    CompressedData QuadInfos;
    CompressedData TriangleIndices;
    CompressedData GroupTriIndices;
} TerrainMesh;

typedef struct{
    TerrainMesh TerrainMesh;
    DynamicArray_TerrainPrimitive TerrainPrimitive;
    TerrainTexture TerrainDisplacementTexture;
    TerrainTexture TerrainNormalTexture;
    TerrainTexture TerrainTriangleMapTexture;
    TerrainTexture TerrainMaterialDuplexTexture;
    TerrainTexture TerrainColorTexture;
    TerrainTexture TerrainQualityTexture;
    TerrainTexture TerrainIndirectionTexture;
    TerrainTexture TerrainSSDFAtlas;
    uint32 DisplacementDownsampled:1;
} TerrainPatch;

typedef enum{ // size: 4
    STREAM_PATCH_STATIC_POOL = 0,
    STREAM_PATCH_DYNAMIC = 1,
    STREAM_PATCH_USER = 2,
} StreamPatchMemoryType;

typedef struct{
    STI_uint32 Version;
    StreamPatchMemoryType MemoryType;
    STI_int32 MemorySize;
    STI_int32 PatchPositionX;
    STI_int32 PatchPositionZ;
    STI_int32 PatchLod;
} StreamPatchBlockHeader;

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
    if (read_STI_uint32(buffer, &count) != BUFFER_SUCCESS) return false;
    DA_init(out, MedianNormal, count);
    for (uint32 i = 0; i < count; ++i) {
        if (!read_MedianNormal(buffer, &out->items[i])) return false;
    }
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
    if (read_STI_uint32(buffer, &count) != BUFFER_SUCCESS) return false;
    DA_init(out, WorldAudioVector4, count);
    for (uint32 i = 0; i < count; ++i) {
        if (!read_WorldAudioVector4(buffer, &out->items[i])) return false;
    }
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
    if (read_STI_uint32(buffer, &count) != BUFFER_SUCCESS) return false;
    DA_init(out, WorldAudioPatchZoneData, count);
    for (uint32 i = 0; i < count; ++i) {
        if (!read_WorldAudioPatchZoneData(buffer, &out->items[i])) return false;
    }
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
    if (read_STI_uint32(buffer, &count) != BUFFER_SUCCESS) return false;
    DA_init(out, ArrayAABB, count);
    for (uint32 i = 0; i < count; ++i) {
        if (!read_ArrayAABB(buffer, &out->items[i])) return false;
    }
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
    if (read_STI_uint32(buffer, &count) != BUFFER_SUCCESS) return false;
    DA_init(out, TerrainPatchInfo, count);
    for (uint32 i = 0; i < count; ++i) {
        if (!read_TerrainPatchInfo(buffer, &out->items[i])) return false;
    }
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
    if (read_STI_uint32(buffer, &count) != BUFFER_SUCCESS) return false;
    DA_init(out, STI_String, count);
    for (uint32 i = 0; i < count; ++i) {
        if (!read_STI_String(buffer, &out->items[i])) return false;
    }
    return true;
}
static bool read_DynamicArray_STI_uint16(Buffer* buffer, DynamicArray_STI_uint16* out) {
    uint32 count = 0;
    if (read_STI_uint32(buffer, &count) != BUFFER_SUCCESS) return false;
    DA_init(out, STI_uint16, count);
    for (uint32 i = 0; i < count; ++i) {
        if (!read_STI_uint16(buffer, &out->items[i])) return false;
    }
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
    if (read_STI_uint32(buffer, &count) != BUFFER_SUCCESS) return false;
    DA_init(out, STI_uint32, count);
    for (uint32 i = 0; i < count; ++i) {
        if (!read_STI_uint32(buffer, &out->items[i])) return false;
    }
    return true;
}
static bool read_VegetationSystemInstance(Buffer* buffer, VegetationSystemInstance* out) {
    if (!read_STI_uint16(buffer, &out->X)) return false;
    if (!read_STI_uint16(buffer, &out->Z)) return false;
    if (!read_STI_uint32(buffer, &out->Rotation)) return false;
    if (!read_STI_uint8(buffer, &out->Color_R)) return false;
    if (!read_STI_uint8(buffer, &out->Color_G)) return false;
    if (!read_STI_uint8(buffer, &out->Color_B)) return false;
    return true;
}
static bool read_DynamicArray_VegetationSystemInstance(Buffer* buffer, DynamicArray_VegetationSystemInstance* out) {
    uint32 count = 0;
    if (read_STI_uint32(buffer, &count) != BUFFER_SUCCESS) return false;
    DA_init(out, VegetationSystemInstance, count);
    for (uint32 i = 0; i < count; ++i) {
        if (!read_VegetationSystemInstance(buffer, &out->items[i])) return false;
    }
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
    if (read_STI_uint32(buffer, &count) != BUFFER_SUCCESS) return false;
    DA_init(out, InstanceDataLayer, count);
    for (uint32 i = 0; i < count; ++i) {
        if (!read_InstanceDataLayer(buffer, &out->items[i])) return false;
    }
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
    if (read_STI_uint32(buffer, &value) != BUFFER_SUCCESS) return false;
    *out = (BlockCompressionType)value;
    return true;
}
static bool read_TerrainTexture(Buffer* buffer, TerrainTexture* out) {
    if (!read_STI_uint32(buffer, &out->Width)) return false;
    if (!read_STI_uint32(buffer, &out->Height)) return false;
    if (!read_BlockCompressionType(buffer, &out->BlockCompressionType)) return false;
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
    if (read_STI_uint32(buffer, &count) != BUFFER_SUCCESS) return false;
    DA_init(out, TerrainPrimitive, count);
    for (uint32 i = 0; i < count; ++i) {
        if (!read_TerrainPrimitive(buffer, &out->items[i])) return false;
    }
    return true;
}
static bool read_MemAllocator(Buffer* buffer, MemAllocator* out) {
    uint32 value = 0;
    if (read_STI_uint32(buffer, &value) != BUFFER_SUCCESS) return false;
    *out = (MemAllocator)value;
    return true;
}
static bool read_DynamicArray_STI_uint8(Buffer* buffer, DynamicArray_STI_uint8* out) {
    uint32 count = 0;
    if (read_STI_uint32(buffer, &count) != BUFFER_SUCCESS) return false;
    DA_init(out, STI_uint8, count);
    for (uint32 i = 0; i < count; ++i) {
        if (!read_STI_uint8(buffer, &out->items[i])) return false;
    }
    return true;
}
static bool read_CompressedData(Buffer* buffer, CompressedData* out) {
    if (!read_STI_uint32(buffer, &out->UncompressedSize)) return false;
    if (!read_DynamicArray_STI_uint8(buffer, &out->Data)) return false;
    if (!read_MemAllocator(buffer, &out->UncompressAllocator)) return false;
    return true;
}
static bool read_TerrainMesh(Buffer* buffer, TerrainMesh* out) {
    for (uint32 i = 0; i < 6; ++i) {
        if (!read_STI_float32(buffer, &out->BoundingBox[i])) return false;
    }
    if (!read_CompressedData(buffer, &out->Indices)) return false;
    if (!read_STI_uint32(buffer, &out->IndexTypeSize)) return false;
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
    return true;
}
static bool read_StreamPatchMemoryType(Buffer* buffer, StreamPatchMemoryType* out) {
    uint32 value = 0;
    if (read_STI_uint32(buffer, &value) != BUFFER_SUCCESS) return false;
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

#endif //APEXPREDATOR_ADF_TYPES_H
