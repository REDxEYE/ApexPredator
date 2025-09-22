// Created by RED on 20.09.2025.

#ifndef APEXPREDATOR_ADF_TYPES_H
#define APEXPREDATOR_ADF_TYPES_H

#include "apex/adf/sti_shared.h"
#include "utils/dynamic_array.h"
DYNAMIC_ARRAY_STRUCT(STI_uint8, STI_uint8);


typedef struct{
    DynamicArray_STI_uint8 d; // offset: 0, size: 16
    STI_uint32 ResourceContainerSizeUnpacked; // offset: 16, size: 4
} ResourceContainerSizeUnpacked; // size: 24



typedef struct{
    STI_float32 Vec[4]; // offset: 0, size: 16
} WorldAudioVector4; // size: 16

DYNAMIC_ARRAY_STRUCT(WorldAudioVector4, WorldAudioVector4);


typedef struct{
    DynamicArray_WorldAudioVector4 Points; // offset: 0, size: 16
    DynamicArray_WorldAudioVector4 PhysicsStreamPatchType; // offset: 16, size: 16
    STI_uint32 ZoneIndex; // offset: 32, size: 4
    STI_float32 MinBounds[3]; // offset: 36, size: 12
    STI_float32 MaxBounds[3]; // offset: 48, size: 12
    STI_float32 ResourceContainer[3]; // offset: 60, size: 12
} MedianNormal; // size: 72

DYNAMIC_ARRAY_STRUCT(MedianNormal, MedianNormal);

typedef struct{
    DynamicArray_MedianNormal ZoneNormalData; // offset: 0, size: 16
} WorldAudioPatchNormalData; // size: 16

typedef struct{
    DynamicArray_WorldAudioVector4 Points; // offset: 0, size: 16
    STI_uint32 ZoneIndex; // offset: 16, size: 4
    STI_float32 MinBounds[3]; // offset: 20, size: 12
    STI_float32 MaxBounds[3]; // offset: 32, size: 12
} WorldAudioPatchZoneData; // size: 48

DYNAMIC_ARRAY_STRUCT(WorldAudioPatchZoneData, WorldAudioPatchZoneData);

typedef struct{
    DynamicArray_WorldAudioPatchZoneData ZoneData; // offset: 0, size: 16
} WorldAudioPatchData; // size: 16



typedef struct{
    STI_uint32 X; // offset: 0, size: 4
    STI_uint32 Z; // offset: 4, size: 4
    STI_uint32 Size; // offset: 8, size: 4
    STI_uint32 Index; // offset: 12, size: 4
    STI_uint32 Count; // offset: 16, size: 4
    STI_int16 Children[4]; // offset: 20, size: 8
} ArrayAABB; // size: 28

DYNAMIC_ARRAY_STRUCT(ArrayAABB, ArrayAABB);


DYNAMIC_ARRAY_STRUCT(STI_uint32, STI_uint32);

typedef struct{
    STI_int32 PatchX; // offset: 0, size: 4
    STI_int32 PatchZ; // offset: 4, size: 4
    STI_int32 PatchLod; // offset: 8, size: 4
    DynamicArray_STI_uint32 TriangleIndices; // offset: 16, size: 16
    DynamicArray_ArrayAABB TriangleQuadTree; // offset: 32, size: 16
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
    DynamicArray_STI_uint16 LocationIndices; // offset: 0, size: 16
    DynamicArray_STI_String LocationNames; // offset: 16, size: 16
    VegetationBillboardLayerStats Stats; // offset: 32, size: 32
} VegetationDebugData; // size: 64

typedef struct{
    STI_uint16 X; // offset: 0, size: 2
    STI_uint16 Z; // offset: 2, size: 2
    uint32 Y:24; // offset: 4, size: 4
    uint8 ZoneIndex:6; // offset: 8, size: 1
    uint8 IsPlanted:1; // offset: 8, size: 1
    uint8 IsDestroyed:1; // offset: 8, size: 1
    STI_uint32 Rotation; // offset: 12, size: 4
    STI_uint8 Color_R; // offset: 16, size: 1
    STI_uint8 Color_G; // offset: 17, size: 1
    STI_uint8 Color_B; // offset: 18, size: 1
    uint32 NameHash; // offset: 20, size: 4
} VegetationSystemInstance; // size: 24

DYNAMIC_ARRAY_STRUCT(VegetationSystemInstance, VegetationSystemInstance);

typedef struct{
    uint32 Name; // offset: 0, size: 4
    STI_float32 BoundingMin[3]; // offset: 4, size: 12
    STI_float32 BoundingMax[3]; // offset: 16, size: 12
    DynamicArray_VegetationSystemInstance Instances; // offset: 32, size: 16
    DynamicArray_STI_uint32 UsedTypes; // offset: 48, size: 16
    VegetationDebugData VegetationDebugData; // offset: 64, size: 64
} InstanceDataLayer; // size: 128

DYNAMIC_ARRAY_STRUCT(InstanceDataLayer, InstanceDataLayer);

typedef struct{
    STI_int32 PatchX; // offset: 0, size: 4
    STI_int32 PatchZ; // offset: 4, size: 4
    STI_int32 PatchLod; // offset: 8, size: 4
    DynamicArray_InstanceDataLayer InstanceDataLayers; // offset: 16, size: 16
    DynamicArray_TerrainPatchInfo TerrainPatchInfo; // offset: 32, size: 16
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

typedef struct{
    STI_uint32 UncompressedSize; // offset: 0, size: 4
    DynamicArray_STI_uint8 Data; // offset: 8, size: 16
    MemAllocator UncompressAllocator; // offset: 24, size: 4
} CompressedData; // size: 32

typedef struct{
    STI_uint32 Width; // offset: 0, size: 4
    STI_uint32 Height; // offset: 4, size: 4
    BlockCompressionType BlockCompressionType; // offset: 8, size: 4
    uint32 Tiled:1; // offset: 12, size: 4
    uint32 Srgb:1; // offset: 12, size: 4
    CompressedData Data; // offset: 16, size: 32
} TerrainTexture; // size: 48

typedef struct{
    STI_float32 AABBMin[3]; // offset: 0, size: 12
    STI_float32 AABBMax[3]; // offset: 12, size: 12
    STI_float32 MinW; // offset: 24, size: 4
    STI_float32 MaxW; // offset: 28, size: 4
} TerrainPrimitive; // size: 32

DYNAMIC_ARRAY_STRUCT(TerrainPrimitive, TerrainPrimitive);


typedef struct{
    STI_float32 BoundingBox[6]; // offset: 0, size: 24
    CompressedData Indices; // offset: 24, size: 32
    STI_uint32 IndexTypeSize; // offset: 56, size: 4
    CompressedData Vertices; // offset: 64, size: 32
    CompressedData Vertices2; // offset: 96, size: 32
    CompressedData QuadInfos; // offset: 128, size: 32
    CompressedData TriangleIndices; // offset: 160, size: 32
    CompressedData GroupTriIndices; // offset: 192, size: 32
} TerrainMesh; // size: 224

typedef struct{
    TerrainMesh TerrainMesh; // offset: 0, size: 224
    DynamicArray_TerrainPrimitive TerrainPrimitive; // offset: 224, size: 16
    TerrainTexture TerrainDisplacementTexture; // offset: 240, size: 48
    TerrainTexture TerrainNormalTexture; // offset: 288, size: 48
    TerrainTexture TerrainTriangleMapTexture; // offset: 336, size: 48
    TerrainTexture TerrainMaterialDuplexTexture; // offset: 384, size: 48
    TerrainTexture TerrainColorTexture; // offset: 432, size: 48
    TerrainTexture TerrainQualityTexture; // offset: 480, size: 48
    TerrainTexture TerrainIndirectionTexture; // offset: 528, size: 48
    TerrainTexture TerrainSSDFAtlas; // offset: 576, size: 48
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

static bool read_ResourceContainerSizeUnpacked(Buffer* buffer, ResourceContainerSizeUnpacked* out);
static void free_ResourceContainerSizeUnpacked(ResourceContainerSizeUnpacked* obj);
static void print_ResourceContainerSizeUnpacked(ResourceContainerSizeUnpacked* obj, FILE* handle, uint32 indent);
static bool read_MedianNormal(Buffer* buffer, MedianNormal* out);
static void free_MedianNormal(MedianNormal* obj);
static void print_MedianNormal(MedianNormal* obj, FILE* handle, uint32 indent);
static bool read_DynamicArray_MedianNormal(Buffer* buffer, DynamicArray_MedianNormal* out);
static void free_DynamicArray_MedianNormal(DynamicArray_MedianNormal* obj);
static void print_DynamicArray_MedianNormal(DynamicArray_MedianNormal* obj, FILE* handle, uint32 indent);
static bool read_WorldAudioPatchNormalData(Buffer* buffer, WorldAudioPatchNormalData* out);
static void free_WorldAudioPatchNormalData(WorldAudioPatchNormalData* obj);
static void print_WorldAudioPatchNormalData(WorldAudioPatchNormalData* obj, FILE* handle, uint32 indent);
static bool read_WorldAudioVector4(Buffer* buffer, WorldAudioVector4* out);
static void free_WorldAudioVector4(WorldAudioVector4* obj);
static void print_WorldAudioVector4(WorldAudioVector4* obj, FILE* handle, uint32 indent);
static bool read_DynamicArray_WorldAudioVector4(Buffer* buffer, DynamicArray_WorldAudioVector4* out);
static void free_DynamicArray_WorldAudioVector4(DynamicArray_WorldAudioVector4* obj);
static void print_DynamicArray_WorldAudioVector4(DynamicArray_WorldAudioVector4* obj, FILE* handle, uint32 indent);
static bool read_WorldAudioPatchZoneData(Buffer* buffer, WorldAudioPatchZoneData* out);
static void free_WorldAudioPatchZoneData(WorldAudioPatchZoneData* obj);
static void print_WorldAudioPatchZoneData(WorldAudioPatchZoneData* obj, FILE* handle, uint32 indent);
static bool read_DynamicArray_WorldAudioPatchZoneData(Buffer* buffer, DynamicArray_WorldAudioPatchZoneData* out);
static void free_DynamicArray_WorldAudioPatchZoneData(DynamicArray_WorldAudioPatchZoneData* obj);
static void print_DynamicArray_WorldAudioPatchZoneData(DynamicArray_WorldAudioPatchZoneData* obj, FILE* handle, uint32 indent);
static bool read_WorldAudioPatchData(Buffer* buffer, WorldAudioPatchData* out);
static void free_WorldAudioPatchData(WorldAudioPatchData* obj);
static void print_WorldAudioPatchData(WorldAudioPatchData* obj, FILE* handle, uint32 indent);
static bool read_ArrayAABB(Buffer* buffer, ArrayAABB* out);
static void free_ArrayAABB(ArrayAABB* obj);
static void print_ArrayAABB(ArrayAABB* obj, FILE* handle, uint32 indent);
static bool read_DynamicArray_ArrayAABB(Buffer* buffer, DynamicArray_ArrayAABB* out);
static void free_DynamicArray_ArrayAABB(DynamicArray_ArrayAABB* obj);
static void print_DynamicArray_ArrayAABB(DynamicArray_ArrayAABB* obj, FILE* handle, uint32 indent);
static bool read_TerrainPatchInfo(Buffer* buffer, TerrainPatchInfo* out);
static void free_TerrainPatchInfo(TerrainPatchInfo* obj);
static void print_TerrainPatchInfo(TerrainPatchInfo* obj, FILE* handle, uint32 indent);
static bool read_DynamicArray_TerrainPatchInfo(Buffer* buffer, DynamicArray_TerrainPatchInfo* out);
static void free_DynamicArray_TerrainPatchInfo(DynamicArray_TerrainPatchInfo* obj);
static void print_DynamicArray_TerrainPatchInfo(DynamicArray_TerrainPatchInfo* obj, FILE* handle, uint32 indent);
static bool read_VegetationBillboardLayerStats(Buffer* buffer, VegetationBillboardLayerStats* out);
static void free_VegetationBillboardLayerStats(VegetationBillboardLayerStats* obj);
static void print_VegetationBillboardLayerStats(VegetationBillboardLayerStats* obj, FILE* handle, uint32 indent);
static bool read_DynamicArray_STI_String(Buffer* buffer, DynamicArray_STI_String* out);
static void free_DynamicArray_STI_String(DynamicArray_STI_String* obj);
static void print_DynamicArray_STI_String(DynamicArray_STI_String* obj, FILE* handle, uint32 indent);
static bool read_DynamicArray_STI_uint16(Buffer* buffer, DynamicArray_STI_uint16* out);
static void free_DynamicArray_STI_uint16(DynamicArray_STI_uint16* obj);
static void print_DynamicArray_STI_uint16(DynamicArray_STI_uint16* obj, FILE* handle, uint32 indent);
static bool read_VegetationDebugData(Buffer* buffer, VegetationDebugData* out);
static void free_VegetationDebugData(VegetationDebugData* obj);
static void print_VegetationDebugData(VegetationDebugData* obj, FILE* handle, uint32 indent);
static bool read_DynamicArray_STI_uint32(Buffer* buffer, DynamicArray_STI_uint32* out);
static void free_DynamicArray_STI_uint32(DynamicArray_STI_uint32* obj);
static void print_DynamicArray_STI_uint32(DynamicArray_STI_uint32* obj, FILE* handle, uint32 indent);
static bool read_VegetationSystemInstance(Buffer* buffer, VegetationSystemInstance* out);
static void free_VegetationSystemInstance(VegetationSystemInstance* obj);
static void print_VegetationSystemInstance(VegetationSystemInstance* obj, FILE* handle, uint32 indent);
static bool read_DynamicArray_VegetationSystemInstance(Buffer* buffer, DynamicArray_VegetationSystemInstance* out);
static void free_DynamicArray_VegetationSystemInstance(DynamicArray_VegetationSystemInstance* obj);
static void print_DynamicArray_VegetationSystemInstance(DynamicArray_VegetationSystemInstance* obj, FILE* handle, uint32 indent);
static bool read_InstanceDataLayer(Buffer* buffer, InstanceDataLayer* out);
static void free_InstanceDataLayer(InstanceDataLayer* obj);
static void print_InstanceDataLayer(InstanceDataLayer* obj, FILE* handle, uint32 indent);
static bool read_DynamicArray_InstanceDataLayer(Buffer* buffer, DynamicArray_InstanceDataLayer* out);
static void free_DynamicArray_InstanceDataLayer(DynamicArray_InstanceDataLayer* obj);
static void print_DynamicArray_InstanceDataLayer(DynamicArray_InstanceDataLayer* obj, FILE* handle, uint32 indent);
static bool read_InstanceDataPatch(Buffer* buffer, InstanceDataPatch* out);
static void free_InstanceDataPatch(InstanceDataPatch* obj);
static void print_InstanceDataPatch(InstanceDataPatch* obj, FILE* handle, uint32 indent);
static bool read_BlockCompressionType(Buffer* buffer, BlockCompressionType* out);
static void free_BlockCompressionType(BlockCompressionType* obj);
static void print_BlockCompressionType(BlockCompressionType* obj, FILE* handle, uint32 indent);
static bool read_TerrainTexture(Buffer* buffer, TerrainTexture* out);
static void free_TerrainTexture(TerrainTexture* obj);
static void print_TerrainTexture(TerrainTexture* obj, FILE* handle, uint32 indent);
static bool read_TerrainPrimitive(Buffer* buffer, TerrainPrimitive* out);
static void free_TerrainPrimitive(TerrainPrimitive* obj);
static void print_TerrainPrimitive(TerrainPrimitive* obj, FILE* handle, uint32 indent);
static bool read_DynamicArray_TerrainPrimitive(Buffer* buffer, DynamicArray_TerrainPrimitive* out);
static void free_DynamicArray_TerrainPrimitive(DynamicArray_TerrainPrimitive* obj);
static void print_DynamicArray_TerrainPrimitive(DynamicArray_TerrainPrimitive* obj, FILE* handle, uint32 indent);
static bool read_MemAllocator(Buffer* buffer, MemAllocator* out);
static void free_MemAllocator(MemAllocator* obj);
static void print_MemAllocator(MemAllocator* obj, FILE* handle, uint32 indent);
static bool read_DynamicArray_STI_uint8(Buffer* buffer, DynamicArray_STI_uint8* out);
static void free_DynamicArray_STI_uint8(DynamicArray_STI_uint8* obj);
static void print_DynamicArray_STI_uint8(DynamicArray_STI_uint8* obj, FILE* handle, uint32 indent);
static bool read_CompressedData(Buffer* buffer, CompressedData* out);
static void free_CompressedData(CompressedData* obj);
static void print_CompressedData(CompressedData* obj, FILE* handle, uint32 indent);
static bool read_TerrainMesh(Buffer* buffer, TerrainMesh* out);
static void free_TerrainMesh(TerrainMesh* obj);
static void print_TerrainMesh(TerrainMesh* obj, FILE* handle, uint32 indent);
static bool read_TerrainPatch(Buffer* buffer, TerrainPatch* out);
static void free_TerrainPatch(TerrainPatch* obj);
static void print_TerrainPatch(TerrainPatch* obj, FILE* handle, uint32 indent);
static bool read_StreamPatchMemoryType(Buffer* buffer, StreamPatchMemoryType* out);
static void free_StreamPatchMemoryType(StreamPatchMemoryType* obj);
static void print_StreamPatchMemoryType(StreamPatchMemoryType* obj, FILE* handle, uint32 indent);
static bool read_StreamPatchBlockHeader(Buffer* buffer, StreamPatchBlockHeader* out);
static void free_StreamPatchBlockHeader(StreamPatchBlockHeader* obj);
static void print_StreamPatchBlockHeader(StreamPatchBlockHeader* obj, FILE* handle, uint32 indent);
static bool read_StreamPatchFileHeader(Buffer* buffer, StreamPatchFileHeader* out);
static void free_StreamPatchFileHeader(StreamPatchFileHeader* obj);
static void print_StreamPatchFileHeader(StreamPatchFileHeader* obj, FILE* handle, uint32 indent);


static bool read_ResourceContainerSizeUnpacked(Buffer* buffer, ResourceContainerSizeUnpacked* out) {
    if (!read_DynamicArray_STI_uint8(buffer, &out->d)) return false;
    if (!read_STI_uint32(buffer, &out->ResourceContainerSizeUnpacked)) return false;
    buffer->skip(buffer, 4);
    return true;
}
static void free_ResourceContainerSizeUnpacked(ResourceContainerSizeUnpacked* obj) {
    free_DynamicArray_STI_uint8(&obj->d);
}
static void print_ResourceContainerSizeUnpacked(ResourceContainerSizeUnpacked* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "ResourceContainerSizeUnpacked {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "d");
    print_DynamicArray_STI_uint8(&obj->d, handle, indent+1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "ResourceContainerSizeUnpacked");
    print_STI_uint32(&obj->ResourceContainerSizeUnpacked, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
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
static void free_MedianNormal(MedianNormal* obj) {
    free_DynamicArray_WorldAudioVector4(&obj->Points);
    free_DynamicArray_WorldAudioVector4(&obj->PhysicsStreamPatchType);
}
static void print_MedianNormal(MedianNormal* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "MedianNormal {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "Points");
    print_DynamicArray_WorldAudioVector4(&obj->Points, handle, indent+1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "PhysicsStreamPatchType");
    print_DynamicArray_WorldAudioVector4(&obj->PhysicsStreamPatchType, handle, indent+1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "ZoneIndex");
    print_STI_uint32(&obj->ZoneIndex, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "MinBounds");
    fprintf(handle, "{\n%*s", (indent+1)*4, "");
    for (uint32 i = 0; i < 3; ++i) {
        print_STI_float32(&obj->MinBounds[i], handle, indent+1);
        if(i < 2){
            fprintf(handle,", \n%*s", (indent+1)*4, "");
        }
    }
    fprintf(handle, "\n%*s}", (indent+1)*4, "");
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "MaxBounds");
    fprintf(handle, "{\n%*s", (indent+1)*4, "");
    for (uint32 i = 0; i < 3; ++i) {
        print_STI_float32(&obj->MaxBounds[i], handle, indent+1);
        if(i < 2){
            fprintf(handle,", \n%*s", (indent+1)*4, "");
        }
    }
    fprintf(handle, "\n%*s}", (indent+1)*4, "");
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "ResourceContainer");
    fprintf(handle, "{\n%*s", (indent+1)*4, "");
    for (uint32 i = 0; i < 3; ++i) {
        print_STI_float32(&obj->ResourceContainer[i], handle, indent+1);
        if(i < 2){
            fprintf(handle,", \n%*s", (indent+1)*4, "");
        }
    }
    fprintf(handle, "\n%*s}", (indent+1)*4, "");
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_DynamicArray_MedianNormal(Buffer* buffer, DynamicArray_MedianNormal* out) {
    uint32 count = 0;
    uint32 unk0 = 0;
    uint32 offset = 0;
    uint32 unk1 = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &unk0)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    if (!read_STI_uint32(buffer, &unk1)) return false;
    DA_init(out, MedianNormal, count);
    if (count>0) {
        int64 original_offset = 0;
        if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
        if(buffer->set_position(buffer, offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
        for (uint32 i = 0; i < count; ++i) {
            if (!read_MedianNormal(buffer, &out->items[i])) return false;
        }
        out->count = count;
        if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    }
    return true;
}
static void free_DynamicArray_MedianNormal(DynamicArray_MedianNormal* obj) {
    for (uint32 i = 0; i < obj->count; ++i) {
        free_MedianNormal(&obj->items[i]);
    }
    DA_free(obj);
}
static void print_DynamicArray_MedianNormal(DynamicArray_MedianNormal* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "DynamicArray_MedianNormal {\n");
    indent++;
    if (obj->count>50) {
        fprintf(handle, "%*s", indent*4, "");
        print_MedianNormal(&obj->items[0], handle, indent+1);
        fprintf(handle, "\n%*s...\n%*s", indent*4, "", indent*4, "");
        print_MedianNormal(&obj->items[obj->count-1], handle, indent+1);
        fprintf(handle, "\n");
    } else {
        fprintf(handle, "%*s", indent*4, "");
        for (uint32 i = 0; i < obj->count; ++i) {
            print_MedianNormal(&obj->items[i], handle, indent+1);
            if(i < obj->count-1){
                fprintf(handle,", \n%*s", indent*4, "");
            }
        }
        fprintf(handle, "\n");
    }
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_WorldAudioPatchNormalData(Buffer* buffer, WorldAudioPatchNormalData* out) {
    if (!read_DynamicArray_MedianNormal(buffer, &out->ZoneNormalData)) return false;
    return true;
}
static void free_WorldAudioPatchNormalData(WorldAudioPatchNormalData* obj) {
    free_DynamicArray_MedianNormal(&obj->ZoneNormalData);
}
static void print_WorldAudioPatchNormalData(WorldAudioPatchNormalData* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "WorldAudioPatchNormalData {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "ZoneNormalData");
    print_DynamicArray_MedianNormal(&obj->ZoneNormalData, handle, indent+1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_WorldAudioVector4(Buffer* buffer, WorldAudioVector4* out) {
    for (uint32 i = 0; i < 4; ++i) {
        if (!read_STI_float32(buffer, &out->Vec[i])) return false;
    }
    return true;
}
static void free_WorldAudioVector4(WorldAudioVector4* obj) {
}
static void print_WorldAudioVector4(WorldAudioVector4* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "WorldAudioVector4 {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "Vec");
    fprintf(handle, "{\n%*s", (indent+1)*4, "");
    for (uint32 i = 0; i < 4; ++i) {
        print_STI_float32(&obj->Vec[i], handle, indent+1);
        if(i < 3){
            fprintf(handle,", \n%*s", (indent+1)*4, "");
        }
    }
    fprintf(handle, "\n%*s}", (indent+1)*4, "");
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_DynamicArray_WorldAudioVector4(Buffer* buffer, DynamicArray_WorldAudioVector4* out) {
    uint32 count = 0;
    uint32 unk0 = 0;
    uint32 offset = 0;
    uint32 unk1 = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &unk0)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    if (!read_STI_uint32(buffer, &unk1)) return false;
    DA_init(out, WorldAudioVector4, count);
    if (count>0) {
        int64 original_offset = 0;
        if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
        if(buffer->set_position(buffer, offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
        for (uint32 i = 0; i < count; ++i) {
            if (!read_WorldAudioVector4(buffer, &out->items[i])) return false;
        }
        out->count = count;
        if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    }
    return true;
}
static void free_DynamicArray_WorldAudioVector4(DynamicArray_WorldAudioVector4* obj) {
    for (uint32 i = 0; i < obj->count; ++i) {
        free_WorldAudioVector4(&obj->items[i]);
    }
    DA_free(obj);
}
static void print_DynamicArray_WorldAudioVector4(DynamicArray_WorldAudioVector4* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "DynamicArray_WorldAudioVector4 {\n");
    indent++;
    if (obj->count>50) {
        fprintf(handle, "%*s", indent*4, "");
        print_WorldAudioVector4(&obj->items[0], handle, indent+1);
        fprintf(handle, "\n%*s...\n%*s", indent*4, "", indent*4, "");
        print_WorldAudioVector4(&obj->items[obj->count-1], handle, indent+1);
        fprintf(handle, "\n");
    } else {
        fprintf(handle, "%*s", indent*4, "");
        for (uint32 i = 0; i < obj->count; ++i) {
            print_WorldAudioVector4(&obj->items[i], handle, indent+1);
            if(i < obj->count-1){
                fprintf(handle,", \n%*s", indent*4, "");
            }
        }
        fprintf(handle, "\n");
    }
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
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
    buffer->skip(buffer, 4);
    return true;
}
static void free_WorldAudioPatchZoneData(WorldAudioPatchZoneData* obj) {
    free_DynamicArray_WorldAudioVector4(&obj->Points);
}
static void print_WorldAudioPatchZoneData(WorldAudioPatchZoneData* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "WorldAudioPatchZoneData {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "Points");
    print_DynamicArray_WorldAudioVector4(&obj->Points, handle, indent+1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "ZoneIndex");
    print_STI_uint32(&obj->ZoneIndex, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "MinBounds");
    fprintf(handle, "{\n%*s", (indent+1)*4, "");
    for (uint32 i = 0; i < 3; ++i) {
        print_STI_float32(&obj->MinBounds[i], handle, indent+1);
        if(i < 2){
            fprintf(handle,", \n%*s", (indent+1)*4, "");
        }
    }
    fprintf(handle, "\n%*s}", (indent+1)*4, "");
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "MaxBounds");
    fprintf(handle, "{\n%*s", (indent+1)*4, "");
    for (uint32 i = 0; i < 3; ++i) {
        print_STI_float32(&obj->MaxBounds[i], handle, indent+1);
        if(i < 2){
            fprintf(handle,", \n%*s", (indent+1)*4, "");
        }
    }
    fprintf(handle, "\n%*s}", (indent+1)*4, "");
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_DynamicArray_WorldAudioPatchZoneData(Buffer* buffer, DynamicArray_WorldAudioPatchZoneData* out) {
    uint32 count = 0;
    uint32 unk0 = 0;
    uint32 offset = 0;
    uint32 unk1 = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &unk0)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    if (!read_STI_uint32(buffer, &unk1)) return false;
    DA_init(out, WorldAudioPatchZoneData, count);
    if (count>0) {
        int64 original_offset = 0;
        if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
        if(buffer->set_position(buffer, offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
        for (uint32 i = 0; i < count; ++i) {
            if (!read_WorldAudioPatchZoneData(buffer, &out->items[i])) return false;
        }
        out->count = count;
        if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    }
    return true;
}
static void free_DynamicArray_WorldAudioPatchZoneData(DynamicArray_WorldAudioPatchZoneData* obj) {
    for (uint32 i = 0; i < obj->count; ++i) {
        free_WorldAudioPatchZoneData(&obj->items[i]);
    }
    DA_free(obj);
}
static void print_DynamicArray_WorldAudioPatchZoneData(DynamicArray_WorldAudioPatchZoneData* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "DynamicArray_WorldAudioPatchZoneData {\n");
    indent++;
    if (obj->count>50) {
        fprintf(handle, "%*s", indent*4, "");
        print_WorldAudioPatchZoneData(&obj->items[0], handle, indent+1);
        fprintf(handle, "\n%*s...\n%*s", indent*4, "", indent*4, "");
        print_WorldAudioPatchZoneData(&obj->items[obj->count-1], handle, indent+1);
        fprintf(handle, "\n");
    } else {
        fprintf(handle, "%*s", indent*4, "");
        for (uint32 i = 0; i < obj->count; ++i) {
            print_WorldAudioPatchZoneData(&obj->items[i], handle, indent+1);
            if(i < obj->count-1){
                fprintf(handle,", \n%*s", indent*4, "");
            }
        }
        fprintf(handle, "\n");
    }
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_WorldAudioPatchData(Buffer* buffer, WorldAudioPatchData* out) {
    if (!read_DynamicArray_WorldAudioPatchZoneData(buffer, &out->ZoneData)) return false;
    return true;
}
static void free_WorldAudioPatchData(WorldAudioPatchData* obj) {
    free_DynamicArray_WorldAudioPatchZoneData(&obj->ZoneData);
}
static void print_WorldAudioPatchData(WorldAudioPatchData* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "WorldAudioPatchData {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "ZoneData");
    print_DynamicArray_WorldAudioPatchZoneData(&obj->ZoneData, handle, indent+1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
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
static void free_ArrayAABB(ArrayAABB* obj) {
}
static void print_ArrayAABB(ArrayAABB* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "ArrayAABB {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "X");
    print_STI_uint32(&obj->X, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Z");
    print_STI_uint32(&obj->Z, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Size");
    print_STI_uint32(&obj->Size, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Index");
    print_STI_uint32(&obj->Index, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Count");
    print_STI_uint32(&obj->Count, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Children");
    fprintf(handle, "{\n%*s", (indent+1)*4, "");
    for (uint32 i = 0; i < 4; ++i) {
        print_STI_int16(&obj->Children[i], handle, indent+1);
        if(i < 3){
            fprintf(handle,", \n%*s", (indent+1)*4, "");
        }
    }
    fprintf(handle, "\n%*s}", (indent+1)*4, "");
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_DynamicArray_ArrayAABB(Buffer* buffer, DynamicArray_ArrayAABB* out) {
    uint32 count = 0;
    uint32 unk0 = 0;
    uint32 offset = 0;
    uint32 unk1 = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &unk0)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    if (!read_STI_uint32(buffer, &unk1)) return false;
    DA_init(out, ArrayAABB, count);
    if (count>0) {
        int64 original_offset = 0;
        if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
        if(buffer->set_position(buffer, offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
        for (uint32 i = 0; i < count; ++i) {
            if (!read_ArrayAABB(buffer, &out->items[i])) return false;
        }
        out->count = count;
        if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    }
    return true;
}
static void free_DynamicArray_ArrayAABB(DynamicArray_ArrayAABB* obj) {
    for (uint32 i = 0; i < obj->count; ++i) {
        free_ArrayAABB(&obj->items[i]);
    }
    DA_free(obj);
}
static void print_DynamicArray_ArrayAABB(DynamicArray_ArrayAABB* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "DynamicArray_ArrayAABB {\n");
    indent++;
    if (obj->count>50) {
        fprintf(handle, "%*s", indent*4, "");
        print_ArrayAABB(&obj->items[0], handle, indent+1);
        fprintf(handle, "\n%*s...\n%*s", indent*4, "", indent*4, "");
        print_ArrayAABB(&obj->items[obj->count-1], handle, indent+1);
        fprintf(handle, "\n");
    } else {
        fprintf(handle, "%*s", indent*4, "");
        for (uint32 i = 0; i < obj->count; ++i) {
            print_ArrayAABB(&obj->items[i], handle, indent+1);
            if(i < obj->count-1){
                fprintf(handle,", \n%*s", indent*4, "");
            }
        }
        fprintf(handle, "\n");
    }
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_TerrainPatchInfo(Buffer* buffer, TerrainPatchInfo* out) {
    if (!read_STI_int32(buffer, &out->PatchX)) return false;
    if (!read_STI_int32(buffer, &out->PatchZ)) return false;
    if (!read_STI_int32(buffer, &out->PatchLod)) return false;
    buffer->skip(buffer, 4);
    if (!read_DynamicArray_STI_uint32(buffer, &out->TriangleIndices)) return false;
    if (!read_DynamicArray_ArrayAABB(buffer, &out->TriangleQuadTree)) return false;
    return true;
}
static void free_TerrainPatchInfo(TerrainPatchInfo* obj) {
    free_DynamicArray_STI_uint32(&obj->TriangleIndices);
    free_DynamicArray_ArrayAABB(&obj->TriangleQuadTree);
}
static void print_TerrainPatchInfo(TerrainPatchInfo* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "TerrainPatchInfo {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "PatchX");
    print_STI_int32(&obj->PatchX, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "PatchZ");
    print_STI_int32(&obj->PatchZ, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "PatchLod");
    print_STI_int32(&obj->PatchLod, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "TriangleIndices");
    print_DynamicArray_STI_uint32(&obj->TriangleIndices, handle, indent+1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "TriangleQuadTree");
    print_DynamicArray_ArrayAABB(&obj->TriangleQuadTree, handle, indent+1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_DynamicArray_TerrainPatchInfo(Buffer* buffer, DynamicArray_TerrainPatchInfo* out) {
    uint32 count = 0;
    uint32 unk0 = 0;
    uint32 offset = 0;
    uint32 unk1 = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &unk0)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    if (!read_STI_uint32(buffer, &unk1)) return false;
    DA_init(out, TerrainPatchInfo, count);
    if (count>0) {
        int64 original_offset = 0;
        if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
        if(buffer->set_position(buffer, offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
        for (uint32 i = 0; i < count; ++i) {
            if (!read_TerrainPatchInfo(buffer, &out->items[i])) return false;
        }
        out->count = count;
        if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    }
    return true;
}
static void free_DynamicArray_TerrainPatchInfo(DynamicArray_TerrainPatchInfo* obj) {
    for (uint32 i = 0; i < obj->count; ++i) {
        free_TerrainPatchInfo(&obj->items[i]);
    }
    DA_free(obj);
}
static void print_DynamicArray_TerrainPatchInfo(DynamicArray_TerrainPatchInfo* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "DynamicArray_TerrainPatchInfo {\n");
    indent++;
    if (obj->count>50) {
        fprintf(handle, "%*s", indent*4, "");
        print_TerrainPatchInfo(&obj->items[0], handle, indent+1);
        fprintf(handle, "\n%*s...\n%*s", indent*4, "", indent*4, "");
        print_TerrainPatchInfo(&obj->items[obj->count-1], handle, indent+1);
        fprintf(handle, "\n");
    } else {
        fprintf(handle, "%*s", indent*4, "");
        for (uint32 i = 0; i < obj->count; ++i) {
            print_TerrainPatchInfo(&obj->items[i], handle, indent+1);
            if(i < obj->count-1){
                fprintf(handle,", \n%*s", indent*4, "");
            }
        }
        fprintf(handle, "\n");
    }
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_VegetationBillboardLayerStats(Buffer* buffer, VegetationBillboardLayerStats* out) {
    if (!read_STI_uint8(buffer, &out->IsRidge)) return false;
    buffer->skip(buffer, 3);
    if (!read_STI_uint32(buffer, &out->NumInstances)) return false;
    if (!read_STI_uint32(buffer, &out->NumOnTreeLine)) return false;
    if (!read_STI_uint32(buffer, &out->NumOnRidges)) return false;
    if (!read_STI_uint32(buffer, &out->NumCulledBySize)) return false;
    if (!read_STI_uint32(buffer, &out->NumCulledByForestMesh)) return false;
    if (!read_STI_uint32(buffer, &out->NumCulledBySeaLevel)) return false;
    if (!read_STI_uint32(buffer, &out->NumCulledByThinout)) return false;
    return true;
}
static void free_VegetationBillboardLayerStats(VegetationBillboardLayerStats* obj) {
}
static void print_VegetationBillboardLayerStats(VegetationBillboardLayerStats* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "VegetationBillboardLayerStats {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "IsRidge");
    print_STI_uint8(&obj->IsRidge, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "NumInstances");
    print_STI_uint32(&obj->NumInstances, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "NumOnTreeLine");
    print_STI_uint32(&obj->NumOnTreeLine, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "NumOnRidges");
    print_STI_uint32(&obj->NumOnRidges, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "NumCulledBySize");
    print_STI_uint32(&obj->NumCulledBySize, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "NumCulledByForestMesh");
    print_STI_uint32(&obj->NumCulledByForestMesh, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "NumCulledBySeaLevel");
    print_STI_uint32(&obj->NumCulledBySeaLevel, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "NumCulledByThinout");
    print_STI_uint32(&obj->NumCulledByThinout, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_DynamicArray_STI_String(Buffer* buffer, DynamicArray_STI_String* out) {
    uint32 count = 0;
    uint32 unk0 = 0;
    uint32 offset = 0;
    uint32 unk1 = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &unk0)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    if (!read_STI_uint32(buffer, &unk1)) return false;
    DA_init(out, STI_String, count);
    if (count>0) {
        int64 original_offset = 0;
        if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
        if(buffer->set_position(buffer, offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
        for (uint32 i = 0; i < count; ++i) {
            if (!read_STI_String(buffer, &out->items[i])) return false;
        }
        out->count = count;
        if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    }
    return true;
}
static void free_DynamicArray_STI_String(DynamicArray_STI_String* obj) {
    for (uint32 i = 0; i < obj->count; ++i) {
        free_STI_String(&obj->items[i]);
    }
    DA_free(obj);
}
static void print_DynamicArray_STI_String(DynamicArray_STI_String* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "DynamicArray_STI_String {\n");
    indent++;
    if (obj->count>50) {
        fprintf(handle, "%*s", indent*4, "");
        print_STI_String(&obj->items[0], handle, indent+1);
        fprintf(handle, "\n%*s...\n%*s", indent*4, "", indent*4, "");
        print_STI_String(&obj->items[obj->count-1], handle, indent+1);
        fprintf(handle, "\n");
    } else {
        fprintf(handle, "%*s", indent*4, "");
        for (uint32 i = 0; i < obj->count; ++i) {
            print_STI_String(&obj->items[i], handle, indent+1);
            if(i < obj->count-1){
                fprintf(handle,", \n%*s", indent*4, "");
            }
        }
        fprintf(handle, "\n");
    }
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_DynamicArray_STI_uint16(Buffer* buffer, DynamicArray_STI_uint16* out) {
    uint32 count = 0;
    uint32 unk0 = 0;
    uint32 offset = 0;
    uint32 unk1 = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &unk0)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    if (!read_STI_uint32(buffer, &unk1)) return false;
    DA_init(out, STI_uint16, count);
    if (count>0) {
        int64 original_offset = 0;
        if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
        if(buffer->set_position(buffer, offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
        for (uint32 i = 0; i < count; ++i) {
            if (!read_STI_uint16(buffer, &out->items[i])) return false;
        }
        out->count = count;
        if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    }
    return true;
}
static void free_DynamicArray_STI_uint16(DynamicArray_STI_uint16* obj) {
    DA_free(obj);
}
static void print_DynamicArray_STI_uint16(DynamicArray_STI_uint16* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "DynamicArray_STI_uint16 {\n");
    indent++;
    if (obj->count>50) {
        fprintf(handle, "%*s", indent*4, "");
        print_STI_uint16(&obj->items[0], handle, indent+1);
        fprintf(handle, "\n%*s...\n%*s", indent*4, "", indent*4, "");
        print_STI_uint16(&obj->items[obj->count-1], handle, indent+1);
        fprintf(handle, "\n");
    } else {
        fprintf(handle, "%*s", indent*4, "");
        for (uint32 i = 0; i < obj->count; ++i) {
            print_STI_uint16(&obj->items[i], handle, indent+1);
            if(i < obj->count-1){
                fprintf(handle,", \n%*s", indent*4, "");
            }
        }
        fprintf(handle, "\n");
    }
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_VegetationDebugData(Buffer* buffer, VegetationDebugData* out) {
    if (!read_DynamicArray_STI_uint16(buffer, &out->LocationIndices)) return false;
    if (!read_DynamicArray_STI_String(buffer, &out->LocationNames)) return false;
    if (!read_VegetationBillboardLayerStats(buffer, &out->Stats)) return false;
    return true;
}
static void free_VegetationDebugData(VegetationDebugData* obj) {
    free_DynamicArray_STI_uint16(&obj->LocationIndices);
    free_DynamicArray_STI_String(&obj->LocationNames);
    free_VegetationBillboardLayerStats(&obj->Stats);
}
static void print_VegetationDebugData(VegetationDebugData* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "VegetationDebugData {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "LocationIndices");
    print_DynamicArray_STI_uint16(&obj->LocationIndices, handle, indent+1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "LocationNames");
    print_DynamicArray_STI_String(&obj->LocationNames, handle, indent+1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Stats");
    print_VegetationBillboardLayerStats(&obj->Stats, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_DynamicArray_STI_uint32(Buffer* buffer, DynamicArray_STI_uint32* out) {
    uint32 count = 0;
    uint32 unk0 = 0;
    uint32 offset = 0;
    uint32 unk1 = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &unk0)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    if (!read_STI_uint32(buffer, &unk1)) return false;
    DA_init(out, STI_uint32, count);
    if (count>0) {
        int64 original_offset = 0;
        if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
        if(buffer->set_position(buffer, offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
        for (uint32 i = 0; i < count; ++i) {
            if (!read_STI_uint32(buffer, &out->items[i])) return false;
        }
        out->count = count;
        if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    }
    return true;
}
static void free_DynamicArray_STI_uint32(DynamicArray_STI_uint32* obj) {
    DA_free(obj);
}
static void print_DynamicArray_STI_uint32(DynamicArray_STI_uint32* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "DynamicArray_STI_uint32 {\n");
    indent++;
    if (obj->count>50) {
        fprintf(handle, "%*s", indent*4, "");
        print_STI_uint32(&obj->items[0], handle, indent+1);
        fprintf(handle, "\n%*s...\n%*s", indent*4, "", indent*4, "");
        print_STI_uint32(&obj->items[obj->count-1], handle, indent+1);
        fprintf(handle, "\n");
    } else {
        fprintf(handle, "%*s", indent*4, "");
        for (uint32 i = 0; i < obj->count; ++i) {
            print_STI_uint32(&obj->items[i], handle, indent+1);
            if(i < obj->count-1){
                fprintf(handle,", \n%*s", indent*4, "");
            }
        }
        fprintf(handle, "\n");
    }
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
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
    buffer->skip(buffer, 4);
    if (!read_STI_uint32(buffer, &out->Rotation)) return false;
    if (!read_STI_uint8(buffer, &out->Color_R)) return false;
    if (!read_STI_uint8(buffer, &out->Color_G)) return false;
    if (!read_STI_uint8(buffer, &out->Color_B)) return false;
    buffer->skip(buffer, 1);
    if (!read_STI_uint32(buffer, &out->NameHash)) return false;
    return true;
}
static void free_VegetationSystemInstance(VegetationSystemInstance* obj) {
}
static void print_VegetationSystemInstance(VegetationSystemInstance* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "VegetationSystemInstance {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "X");
    print_STI_uint16(&obj->X, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Z");
    print_STI_uint16(&obj->Z, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Y");
    fprintf(handle, "%s: %i", "Y", obj->Y);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "ZoneIndex");
    fprintf(handle, "%s: %i", "ZoneIndex", obj->ZoneIndex);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "IsPlanted");
    fprintf(handle, "%s: %i", "IsPlanted", obj->IsPlanted);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "IsDestroyed");
    fprintf(handle, "%s: %i", "IsDestroyed", obj->IsDestroyed);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Rotation");
    print_STI_uint32(&obj->Rotation, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Color_R");
    print_STI_uint8(&obj->Color_R, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Color_G");
    print_STI_uint8(&obj->Color_G, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Color_B");
    print_STI_uint8(&obj->Color_B, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "NameHash");
    fprintf(handle, "%s: %08X", "NameHash", obj->NameHash);
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_DynamicArray_VegetationSystemInstance(Buffer* buffer, DynamicArray_VegetationSystemInstance* out) {
    uint32 count = 0;
    uint32 unk0 = 0;
    uint32 offset = 0;
    uint32 unk1 = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &unk0)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    if (!read_STI_uint32(buffer, &unk1)) return false;
    DA_init(out, VegetationSystemInstance, count);
    if (count>0) {
        int64 original_offset = 0;
        if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
        if(buffer->set_position(buffer, offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
        for (uint32 i = 0; i < count; ++i) {
            if (!read_VegetationSystemInstance(buffer, &out->items[i])) return false;
        }
        out->count = count;
        if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    }
    return true;
}
static void free_DynamicArray_VegetationSystemInstance(DynamicArray_VegetationSystemInstance* obj) {
    for (uint32 i = 0; i < obj->count; ++i) {
        free_VegetationSystemInstance(&obj->items[i]);
    }
    DA_free(obj);
}
static void print_DynamicArray_VegetationSystemInstance(DynamicArray_VegetationSystemInstance* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "DynamicArray_VegetationSystemInstance {\n");
    indent++;
    if (obj->count>50) {
        fprintf(handle, "%*s", indent*4, "");
        print_VegetationSystemInstance(&obj->items[0], handle, indent+1);
        fprintf(handle, "\n%*s...\n%*s", indent*4, "", indent*4, "");
        print_VegetationSystemInstance(&obj->items[obj->count-1], handle, indent+1);
        fprintf(handle, "\n");
    } else {
        fprintf(handle, "%*s", indent*4, "");
        for (uint32 i = 0; i < obj->count; ++i) {
            print_VegetationSystemInstance(&obj->items[i], handle, indent+1);
            if(i < obj->count-1){
                fprintf(handle,", \n%*s", indent*4, "");
            }
        }
        fprintf(handle, "\n");
    }
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_InstanceDataLayer(Buffer* buffer, InstanceDataLayer* out) {
    if (!read_STI_uint32(buffer, &out->Name)) return false;
    for (uint32 i = 0; i < 3; ++i) {
        if (!read_STI_float32(buffer, &out->BoundingMin[i])) return false;
    }
    for (uint32 i = 0; i < 3; ++i) {
        if (!read_STI_float32(buffer, &out->BoundingMax[i])) return false;
    }
    buffer->skip(buffer, 4);
    if (!read_DynamicArray_VegetationSystemInstance(buffer, &out->Instances)) return false;
    if (!read_DynamicArray_STI_uint32(buffer, &out->UsedTypes)) return false;
    if (!read_VegetationDebugData(buffer, &out->VegetationDebugData)) return false;
    return true;
}
static void free_InstanceDataLayer(InstanceDataLayer* obj) {
    free_DynamicArray_VegetationSystemInstance(&obj->Instances);
    free_DynamicArray_STI_uint32(&obj->UsedTypes);
    free_VegetationDebugData(&obj->VegetationDebugData);
}
static void print_InstanceDataLayer(InstanceDataLayer* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "InstanceDataLayer {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "Name");
    fprintf(handle, "%s: %08X", "Name", obj->Name);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "BoundingMin");
    fprintf(handle, "{\n%*s", (indent+1)*4, "");
    for (uint32 i = 0; i < 3; ++i) {
        print_STI_float32(&obj->BoundingMin[i], handle, indent+1);
        if(i < 2){
            fprintf(handle,", \n%*s", (indent+1)*4, "");
        }
    }
    fprintf(handle, "\n%*s}", (indent+1)*4, "");
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "BoundingMax");
    fprintf(handle, "{\n%*s", (indent+1)*4, "");
    for (uint32 i = 0; i < 3; ++i) {
        print_STI_float32(&obj->BoundingMax[i], handle, indent+1);
        if(i < 2){
            fprintf(handle,", \n%*s", (indent+1)*4, "");
        }
    }
    fprintf(handle, "\n%*s}", (indent+1)*4, "");
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Instances");
    print_DynamicArray_VegetationSystemInstance(&obj->Instances, handle, indent+1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "UsedTypes");
    print_DynamicArray_STI_uint32(&obj->UsedTypes, handle, indent+1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "VegetationDebugData");
    print_VegetationDebugData(&obj->VegetationDebugData, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_DynamicArray_InstanceDataLayer(Buffer* buffer, DynamicArray_InstanceDataLayer* out) {
    uint32 count = 0;
    uint32 unk0 = 0;
    uint32 offset = 0;
    uint32 unk1 = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &unk0)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    if (!read_STI_uint32(buffer, &unk1)) return false;
    DA_init(out, InstanceDataLayer, count);
    if (count>0) {
        int64 original_offset = 0;
        if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
        if(buffer->set_position(buffer, offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
        for (uint32 i = 0; i < count; ++i) {
            if (!read_InstanceDataLayer(buffer, &out->items[i])) return false;
        }
        out->count = count;
        if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    }
    return true;
}
static void free_DynamicArray_InstanceDataLayer(DynamicArray_InstanceDataLayer* obj) {
    for (uint32 i = 0; i < obj->count; ++i) {
        free_InstanceDataLayer(&obj->items[i]);
    }
    DA_free(obj);
}
static void print_DynamicArray_InstanceDataLayer(DynamicArray_InstanceDataLayer* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "DynamicArray_InstanceDataLayer {\n");
    indent++;
    if (obj->count>50) {
        fprintf(handle, "%*s", indent*4, "");
        print_InstanceDataLayer(&obj->items[0], handle, indent+1);
        fprintf(handle, "\n%*s...\n%*s", indent*4, "", indent*4, "");
        print_InstanceDataLayer(&obj->items[obj->count-1], handle, indent+1);
        fprintf(handle, "\n");
    } else {
        fprintf(handle, "%*s", indent*4, "");
        for (uint32 i = 0; i < obj->count; ++i) {
            print_InstanceDataLayer(&obj->items[i], handle, indent+1);
            if(i < obj->count-1){
                fprintf(handle,", \n%*s", indent*4, "");
            }
        }
        fprintf(handle, "\n");
    }
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_InstanceDataPatch(Buffer* buffer, InstanceDataPatch* out) {
    if (!read_STI_int32(buffer, &out->PatchX)) return false;
    if (!read_STI_int32(buffer, &out->PatchZ)) return false;
    if (!read_STI_int32(buffer, &out->PatchLod)) return false;
    buffer->skip(buffer, 4);
    if (!read_DynamicArray_InstanceDataLayer(buffer, &out->InstanceDataLayers)) return false;
    if (!read_DynamicArray_TerrainPatchInfo(buffer, &out->TerrainPatchInfo)) return false;
    return true;
}
static void free_InstanceDataPatch(InstanceDataPatch* obj) {
    free_DynamicArray_InstanceDataLayer(&obj->InstanceDataLayers);
    free_DynamicArray_TerrainPatchInfo(&obj->TerrainPatchInfo);
}
static void print_InstanceDataPatch(InstanceDataPatch* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "InstanceDataPatch {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "PatchX");
    print_STI_int32(&obj->PatchX, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "PatchZ");
    print_STI_int32(&obj->PatchZ, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "PatchLod");
    print_STI_int32(&obj->PatchLod, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "InstanceDataLayers");
    print_DynamicArray_InstanceDataLayer(&obj->InstanceDataLayers, handle, indent+1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "TerrainPatchInfo");
    print_DynamicArray_TerrainPatchInfo(&obj->TerrainPatchInfo, handle, indent+1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_BlockCompressionType(Buffer* buffer, BlockCompressionType* out) {
    uint32 value = 0;
    if (!read_STI_uint32(buffer, &value)) return false;
    *out = (BlockCompressionType)value;
    return true;
}
static void free_BlockCompressionType(BlockCompressionType* obj) {
}
static void print_BlockCompressionType(BlockCompressionType* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "BlockCompressionType {\n");
    indent++;
    fprintf(handle, "%*s%s", indent * 4, "", "BlockCompressionType");
    fprintf(handle, " (%i)", (int)*obj);
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_TerrainTexture(Buffer* buffer, TerrainTexture* out) {
    if (!read_STI_uint32(buffer, &out->Width)) return false;
    if (!read_STI_uint32(buffer, &out->Height)) return false;
    if (!read_BlockCompressionType(buffer, &out->BlockCompressionType)) return false;
    uint32 bitfield_value = 0;
    if (!read_STI_uint32(buffer, &bitfield_value)) return false;
    out->Tiled = (bitfield_value >> 0) & 0x1;
    out->Srgb = (bitfield_value >> 1) & 0x1;
    if (!read_CompressedData(buffer, &out->Data)) return false;
    return true;
}
static void free_TerrainTexture(TerrainTexture* obj) {
    free_BlockCompressionType(&obj->BlockCompressionType);
    free_CompressedData(&obj->Data);
}
static void print_TerrainTexture(TerrainTexture* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "TerrainTexture {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "Width");
    print_STI_uint32(&obj->Width, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Height");
    print_STI_uint32(&obj->Height, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "BlockCompressionType");
    fprintf(handle, "%s", "TerrainTexture");
    fprintf(handle, " (%i)", (int)obj->BlockCompressionType);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Tiled");
    fprintf(handle, "%s: %i", "Tiled", obj->Tiled);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Srgb");
    fprintf(handle, "%s: %i", "Srgb", obj->Srgb);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Data");
    print_CompressedData(&obj->Data, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
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
static void free_TerrainPrimitive(TerrainPrimitive* obj) {
}
static void print_TerrainPrimitive(TerrainPrimitive* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "TerrainPrimitive {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "AABBMin");
    fprintf(handle, "{\n%*s", (indent+1)*4, "");
    for (uint32 i = 0; i < 3; ++i) {
        print_STI_float32(&obj->AABBMin[i], handle, indent+1);
        if(i < 2){
            fprintf(handle,", \n%*s", (indent+1)*4, "");
        }
    }
    fprintf(handle, "\n%*s}", (indent+1)*4, "");
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "AABBMax");
    fprintf(handle, "{\n%*s", (indent+1)*4, "");
    for (uint32 i = 0; i < 3; ++i) {
        print_STI_float32(&obj->AABBMax[i], handle, indent+1);
        if(i < 2){
            fprintf(handle,", \n%*s", (indent+1)*4, "");
        }
    }
    fprintf(handle, "\n%*s}", (indent+1)*4, "");
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "MinW");
    print_STI_float32(&obj->MinW, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "MaxW");
    print_STI_float32(&obj->MaxW, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_DynamicArray_TerrainPrimitive(Buffer* buffer, DynamicArray_TerrainPrimitive* out) {
    uint32 count = 0;
    uint32 unk0 = 0;
    uint32 offset = 0;
    uint32 unk1 = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &unk0)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    if (!read_STI_uint32(buffer, &unk1)) return false;
    DA_init(out, TerrainPrimitive, count);
    if (count>0) {
        int64 original_offset = 0;
        if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
        if(buffer->set_position(buffer, offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
        for (uint32 i = 0; i < count; ++i) {
            if (!read_TerrainPrimitive(buffer, &out->items[i])) return false;
        }
        out->count = count;
        if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    }
    return true;
}
static void free_DynamicArray_TerrainPrimitive(DynamicArray_TerrainPrimitive* obj) {
    for (uint32 i = 0; i < obj->count; ++i) {
        free_TerrainPrimitive(&obj->items[i]);
    }
    DA_free(obj);
}
static void print_DynamicArray_TerrainPrimitive(DynamicArray_TerrainPrimitive* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "DynamicArray_TerrainPrimitive {\n");
    indent++;
    if (obj->count>50) {
        fprintf(handle, "%*s", indent*4, "");
        print_TerrainPrimitive(&obj->items[0], handle, indent+1);
        fprintf(handle, "\n%*s...\n%*s", indent*4, "", indent*4, "");
        print_TerrainPrimitive(&obj->items[obj->count-1], handle, indent+1);
        fprintf(handle, "\n");
    } else {
        fprintf(handle, "%*s", indent*4, "");
        for (uint32 i = 0; i < obj->count; ++i) {
            print_TerrainPrimitive(&obj->items[i], handle, indent+1);
            if(i < obj->count-1){
                fprintf(handle,", \n%*s", indent*4, "");
            }
        }
        fprintf(handle, "\n");
    }
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_MemAllocator(Buffer* buffer, MemAllocator* out) {
    uint32 value = 0;
    if (!read_STI_uint32(buffer, &value)) return false;
    *out = (MemAllocator)value;
    return true;
}
static void free_MemAllocator(MemAllocator* obj) {
}
static void print_MemAllocator(MemAllocator* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "MemAllocator {\n");
    indent++;
    fprintf(handle, "%*s%s", indent * 4, "", "MemAllocator");
    fprintf(handle, " (%i)", (int)*obj);
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_DynamicArray_STI_uint8(Buffer* buffer, DynamicArray_STI_uint8* out) {
    uint32 count = 0;
    uint32 unk0 = 0;
    uint32 offset = 0;
    uint32 unk1 = 0;
    if (!read_STI_uint32(buffer, &offset)) return false;
    if (!read_STI_uint32(buffer, &unk0)) return false;
    if (!read_STI_uint32(buffer, &count)) return false;
    if (!read_STI_uint32(buffer, &unk1)) return false;
    DA_init(out, STI_uint8, count);
    if (count>0) {
        int64 original_offset = 0;
        if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;
        if(buffer->set_position(buffer, offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
        for (uint32 i = 0; i < count; ++i) {
            if (!read_STI_uint8(buffer, &out->items[i])) return false;
        }
        out->count = count;
        if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;
    }
    return true;
}
static void free_DynamicArray_STI_uint8(DynamicArray_STI_uint8* obj) {
    DA_free(obj);
}
static void print_DynamicArray_STI_uint8(DynamicArray_STI_uint8* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "DynamicArray_STI_uint8 {\n");
    indent++;
    if (obj->count>50) {
        fprintf(handle, "%*s", indent*4, "");
        print_STI_uint8(&obj->items[0], handle, indent+1);
        fprintf(handle, "\n%*s...\n%*s", indent*4, "", indent*4, "");
        print_STI_uint8(&obj->items[obj->count-1], handle, indent+1);
        fprintf(handle, "\n");
    } else {
        fprintf(handle, "%*s", indent*4, "");
        for (uint32 i = 0; i < obj->count; ++i) {
            print_STI_uint8(&obj->items[i], handle, indent+1);
            if(i < obj->count-1){
                fprintf(handle,", \n%*s", indent*4, "");
            }
        }
        fprintf(handle, "\n");
    }
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_CompressedData(Buffer* buffer, CompressedData* out) {
    if (!read_STI_uint32(buffer, &out->UncompressedSize)) return false;
    buffer->skip(buffer, 4);
    if (!read_DynamicArray_STI_uint8(buffer, &out->Data)) return false;
    if (!read_MemAllocator(buffer, &out->UncompressAllocator)) return false;
    buffer->skip(buffer, 4);
    return true;
}
static void free_CompressedData(CompressedData* obj) {
    free_DynamicArray_STI_uint8(&obj->Data);
    free_MemAllocator(&obj->UncompressAllocator);
}
static void print_CompressedData(CompressedData* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "CompressedData {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "UncompressedSize");
    print_STI_uint32(&obj->UncompressedSize, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Data");
    print_DynamicArray_STI_uint8(&obj->Data, handle, indent+1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "UncompressAllocator");
    fprintf(handle, "%s", "CompressedData");
    fprintf(handle, " (%i)", (int)obj->UncompressAllocator);
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_TerrainMesh(Buffer* buffer, TerrainMesh* out) {
    for (uint32 i = 0; i < 6; ++i) {
        if (!read_STI_float32(buffer, &out->BoundingBox[i])) return false;
    }
    if (!read_CompressedData(buffer, &out->Indices)) return false;
    if (!read_STI_uint32(buffer, &out->IndexTypeSize)) return false;
    buffer->skip(buffer, 4);
    if (!read_CompressedData(buffer, &out->Vertices)) return false;
    if (!read_CompressedData(buffer, &out->Vertices2)) return false;
    if (!read_CompressedData(buffer, &out->QuadInfos)) return false;
    if (!read_CompressedData(buffer, &out->TriangleIndices)) return false;
    if (!read_CompressedData(buffer, &out->GroupTriIndices)) return false;
    return true;
}
static void free_TerrainMesh(TerrainMesh* obj) {
    free_CompressedData(&obj->Indices);
    free_CompressedData(&obj->Vertices);
    free_CompressedData(&obj->Vertices2);
    free_CompressedData(&obj->QuadInfos);
    free_CompressedData(&obj->TriangleIndices);
    free_CompressedData(&obj->GroupTriIndices);
}
static void print_TerrainMesh(TerrainMesh* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "TerrainMesh {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "BoundingBox");
    fprintf(handle, "{\n%*s", (indent+1)*4, "");
    for (uint32 i = 0; i < 6; ++i) {
        print_STI_float32(&obj->BoundingBox[i], handle, indent+1);
        if(i < 5){
            fprintf(handle,", \n%*s", (indent+1)*4, "");
        }
    }
    fprintf(handle, "\n%*s}", (indent+1)*4, "");
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Indices");
    print_CompressedData(&obj->Indices, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "IndexTypeSize");
    print_STI_uint32(&obj->IndexTypeSize, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Vertices");
    print_CompressedData(&obj->Vertices, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Vertices2");
    print_CompressedData(&obj->Vertices2, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "QuadInfos");
    print_CompressedData(&obj->QuadInfos, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "TriangleIndices");
    print_CompressedData(&obj->TriangleIndices, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "GroupTriIndices");
    print_CompressedData(&obj->GroupTriIndices, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
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
    uint32 bitfield_value = 0;
    if (!read_STI_uint32(buffer, &bitfield_value)) return false;
    out->DisplacementDownsampled = (bitfield_value >> 0) & 0x1;
    buffer->skip(buffer, 4);
    return true;
}
static void free_TerrainPatch(TerrainPatch* obj) {
    free_TerrainMesh(&obj->TerrainMesh);
    free_DynamicArray_TerrainPrimitive(&obj->TerrainPrimitive);
    free_TerrainTexture(&obj->TerrainDisplacementTexture);
    free_TerrainTexture(&obj->TerrainNormalTexture);
    free_TerrainTexture(&obj->TerrainTriangleMapTexture);
    free_TerrainTexture(&obj->TerrainMaterialDuplexTexture);
    free_TerrainTexture(&obj->TerrainColorTexture);
    free_TerrainTexture(&obj->TerrainQualityTexture);
    free_TerrainTexture(&obj->TerrainIndirectionTexture);
    free_TerrainTexture(&obj->TerrainSSDFAtlas);
}
static void print_TerrainPatch(TerrainPatch* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "TerrainPatch {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "TerrainMesh");
    print_TerrainMesh(&obj->TerrainMesh, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "TerrainPrimitive");
    print_DynamicArray_TerrainPrimitive(&obj->TerrainPrimitive, handle, indent+1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "TerrainDisplacementTexture");
    print_TerrainTexture(&obj->TerrainDisplacementTexture, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "TerrainNormalTexture");
    print_TerrainTexture(&obj->TerrainNormalTexture, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "TerrainTriangleMapTexture");
    print_TerrainTexture(&obj->TerrainTriangleMapTexture, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "TerrainMaterialDuplexTexture");
    print_TerrainTexture(&obj->TerrainMaterialDuplexTexture, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "TerrainColorTexture");
    print_TerrainTexture(&obj->TerrainColorTexture, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "TerrainQualityTexture");
    print_TerrainTexture(&obj->TerrainQualityTexture, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "TerrainIndirectionTexture");
    print_TerrainTexture(&obj->TerrainIndirectionTexture, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "TerrainSSDFAtlas");
    print_TerrainTexture(&obj->TerrainSSDFAtlas, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "DisplacementDownsampled");
    fprintf(handle, "%s: %i", "DisplacementDownsampled", obj->DisplacementDownsampled);
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static bool read_StreamPatchMemoryType(Buffer* buffer, StreamPatchMemoryType* out) {
    uint32 value = 0;
    if (!read_STI_uint32(buffer, &value)) return false;
    *out = (StreamPatchMemoryType)value;
    return true;
}
static void free_StreamPatchMemoryType(StreamPatchMemoryType* obj) {
}
static void print_StreamPatchMemoryType(StreamPatchMemoryType* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "StreamPatchMemoryType {\n");
    indent++;
    fprintf(handle, "%*s%s", indent * 4, "", "StreamPatchMemoryType");
    fprintf(handle, " (%i)", (int)*obj);
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
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
static void free_StreamPatchBlockHeader(StreamPatchBlockHeader* obj) {
    free_StreamPatchMemoryType(&obj->MemoryType);
}
static void print_StreamPatchBlockHeader(StreamPatchBlockHeader* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "StreamPatchBlockHeader {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "Version");
    print_STI_uint32(&obj->Version, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "MemoryType");
    fprintf(handle, "%s", "StreamPatchBlockHeader");
    fprintf(handle, " (%i)", (int)obj->MemoryType);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "MemorySize");
    print_STI_int32(&obj->MemorySize, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "PatchPositionX");
    print_STI_int32(&obj->PatchPositionX, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "PatchPositionZ");
    print_STI_int32(&obj->PatchPositionZ, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "PatchLod");
    print_STI_int32(&obj->PatchLod, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
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
static void free_StreamPatchFileHeader(StreamPatchFileHeader* obj) {
}
static void print_StreamPatchFileHeader(StreamPatchFileHeader* obj, FILE* handle, uint32 indent) {
    fprintf(handle, "StreamPatchFileHeader {\n");
    indent++;
    fprintf(handle, "%*s%s = ", indent * 4, "", "Version");
    print_STI_uint32(&obj->Version, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "Size");
    print_STI_int32(&obj->Size, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "DynamicMemoryRequirements");
    print_STI_uint32(&obj->DynamicMemoryRequirements, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "PatchPositionX");
    print_STI_int32(&obj->PatchPositionX, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "PatchPositionZ");
    print_STI_int32(&obj->PatchPositionZ, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s%s = ", indent * 4, "", "PatchLod");
    print_STI_int32(&obj->PatchLod, handle, indent + 1);
    fprintf(handle, "\n");
    fprintf(handle, "%*s}", (indent - 1) * 4, "");
}
static void STI_ADF_register_functions(STI_TypeLibrary* lib) {
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x0D811C0C))) = (STI_ObjectMethods){(readSTIObject)read_TerrainTexture, (freeSTIObject)free_TerrainTexture, (printSTIObject)print_TerrainTexture};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x165A5795))) = (STI_ObjectMethods){(readSTIObject)read_DynamicArray_WorldAudioPatchZoneData, (freeSTIObject)free_DynamicArray_WorldAudioPatchZoneData, (printSTIObject)print_DynamicArray_WorldAudioPatchZoneData};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x1F783B17))) = (STI_ObjectMethods){(readSTIObject)read_VegetationSystemInstance, (freeSTIObject)free_VegetationSystemInstance, (printSTIObject)print_VegetationSystemInstance};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x200D0BC9))) = (STI_ObjectMethods){(readSTIObject)read_ArrayAABB, (freeSTIObject)free_ArrayAABB, (printSTIObject)print_ArrayAABB};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x21A16FA4))) = (STI_ObjectMethods){(readSTIObject)read_WorldAudioVector4, (freeSTIObject)free_WorldAudioVector4, (printSTIObject)print_WorldAudioVector4};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x2EEB55AC))) = (STI_ObjectMethods){(readSTIObject)read_MedianNormal, (freeSTIObject)free_MedianNormal, (printSTIObject)print_MedianNormal};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x45FEA6F5))) = (STI_ObjectMethods){(readSTIObject)read_TerrainPrimitive, (freeSTIObject)free_TerrainPrimitive, (printSTIObject)print_TerrainPrimitive};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x52EEFC8D))) = (STI_ObjectMethods){(readSTIObject)read_MemAllocator, (freeSTIObject)free_MemAllocator, (printSTIObject)print_MemAllocator};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x56500CAA))) = (STI_ObjectMethods){(readSTIObject)read_CompressedData, (freeSTIObject)free_CompressedData, (printSTIObject)print_CompressedData};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x573FA4BF))) = (STI_ObjectMethods){(readSTIObject)read_DynamicArray_ArrayAABB, (freeSTIObject)free_DynamicArray_ArrayAABB, (printSTIObject)print_DynamicArray_ArrayAABB};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x5A6DE0C2))) = (STI_ObjectMethods){(readSTIObject)read_StreamPatchMemoryType, (freeSTIObject)free_StreamPatchMemoryType, (printSTIObject)print_StreamPatchMemoryType};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x5EE422D0))) = (STI_ObjectMethods){(readSTIObject)read_DynamicArray_WorldAudioVector4, (freeSTIObject)free_DynamicArray_WorldAudioVector4, (printSTIObject)print_DynamicArray_WorldAudioVector4};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x6032D156))) = (STI_ObjectMethods){(readSTIObject)read_DynamicArray_TerrainPatchInfo, (freeSTIObject)free_DynamicArray_TerrainPatchInfo, (printSTIObject)print_DynamicArray_TerrainPatchInfo};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x65950F52))) = (STI_ObjectMethods){(readSTIObject)read_DynamicArray_MedianNormal, (freeSTIObject)free_DynamicArray_MedianNormal, (printSTIObject)print_DynamicArray_MedianNormal};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x6D5409F0))) = (STI_ObjectMethods){(readSTIObject)read_WorldAudioPatchNormalData, (freeSTIObject)free_WorldAudioPatchNormalData, (printSTIObject)print_WorldAudioPatchNormalData};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x6F962A30))) = (STI_ObjectMethods){(readSTIObject)read_DynamicArray_STI_uint8, (freeSTIObject)free_DynamicArray_STI_uint8, (printSTIObject)print_DynamicArray_STI_uint8};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x72E3A69A))) = (STI_ObjectMethods){(readSTIObject)read_DynamicArray_STI_uint16, (freeSTIObject)free_DynamicArray_STI_uint16, (printSTIObject)print_DynamicArray_STI_uint16};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x766E412E))) = (STI_ObjectMethods){(readSTIObject)read_DynamicArray_InstanceDataLayer, (freeSTIObject)free_DynamicArray_InstanceDataLayer, (printSTIObject)print_DynamicArray_InstanceDataLayer};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x78CB76FD))) = (STI_ObjectMethods){(readSTIObject)read_StreamPatchBlockHeader, (freeSTIObject)free_StreamPatchBlockHeader, (printSTIObject)print_StreamPatchBlockHeader};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x80B0FEC7))) = (STI_ObjectMethods){(readSTIObject)read_DynamicArray_TerrainPrimitive, (freeSTIObject)free_DynamicArray_TerrainPrimitive, (printSTIObject)print_DynamicArray_TerrainPrimitive};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x852D0D20))) = (STI_ObjectMethods){(readSTIObject)read_StreamPatchFileHeader, (freeSTIObject)free_StreamPatchFileHeader, (printSTIObject)print_StreamPatchFileHeader};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x962828FC))) = (STI_ObjectMethods){(readSTIObject)read_TerrainMesh, (freeSTIObject)free_TerrainMesh, (printSTIObject)print_TerrainMesh};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0xA553A921))) = (STI_ObjectMethods){(readSTIObject)read_VegetationBillboardLayerStats, (freeSTIObject)free_VegetationBillboardLayerStats, (printSTIObject)print_VegetationBillboardLayerStats};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0xB057437A))) = (STI_ObjectMethods){(readSTIObject)read_WorldAudioPatchData, (freeSTIObject)free_WorldAudioPatchData, (printSTIObject)print_WorldAudioPatchData};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0xB9D80AD4))) = (STI_ObjectMethods){(readSTIObject)read_TerrainPatchInfo, (freeSTIObject)free_TerrainPatchInfo, (printSTIObject)print_TerrainPatchInfo};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0xC106B357))) = (STI_ObjectMethods){(readSTIObject)read_DynamicArray_STI_String, (freeSTIObject)free_DynamicArray_STI_String, (printSTIObject)print_DynamicArray_STI_String};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0xC49009EE))) = (STI_ObjectMethods){(readSTIObject)read_ResourceContainerSizeUnpacked, (freeSTIObject)free_ResourceContainerSizeUnpacked, (printSTIObject)print_ResourceContainerSizeUnpacked};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0xC7D58849))) = (STI_ObjectMethods){(readSTIObject)read_VegetationDebugData, (freeSTIObject)free_VegetationDebugData, (printSTIObject)print_VegetationDebugData};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0xD7FF1C92))) = (STI_ObjectMethods){(readSTIObject)read_WorldAudioPatchZoneData, (freeSTIObject)free_WorldAudioPatchZoneData, (printSTIObject)print_WorldAudioPatchZoneData};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0xD97AAC30))) = (STI_ObjectMethods){(readSTIObject)read_BlockCompressionType, (freeSTIObject)free_BlockCompressionType, (printSTIObject)print_BlockCompressionType};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0xE8ABFE3B))) = (STI_ObjectMethods){(readSTIObject)read_DynamicArray_STI_uint32, (freeSTIObject)free_DynamicArray_STI_uint32, (printSTIObject)print_DynamicArray_STI_uint32};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0xEA0A66F0))) = (STI_ObjectMethods){(readSTIObject)read_DynamicArray_VegetationSystemInstance, (freeSTIObject)free_DynamicArray_VegetationSystemInstance, (printSTIObject)print_DynamicArray_VegetationSystemInstance};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0xF8DD7138))) = (STI_ObjectMethods){(readSTIObject)read_InstanceDataLayer, (freeSTIObject)free_InstanceDataLayer, (printSTIObject)print_InstanceDataLayer};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0xFD31E1DB))) = (STI_ObjectMethods){(readSTIObject)read_TerrainPatch, (freeSTIObject)free_TerrainPatch, (printSTIObject)print_TerrainPatch};
    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0xFD88832A))) = (STI_ObjectMethods){(readSTIObject)read_InstanceDataPatch, (freeSTIObject)free_InstanceDataPatch, (printSTIObject)print_InstanceDataPatch};
}

#endif //APEXPREDATOR_ADF_TYPES_H
