// Created by RED on 30.09.2025.

#ifndef APEXPREDATOR_AVTX_H
#define APEXPREDATOR_AVTX_H
#include "int_def.h"
#include "platform/texture.h"
#include "utils/buffer/buffer.h"

#define AVTX_MAGIC "AVTX"

#pragma pack(push, 1)
typedef struct {
    uint32 offset;
    uint32 size;
    uint16 alignment;
    uint8 tile_mode;
    uint8 source;
} AVTXStream;

typedef enum AVATextureFlag {
    E_AVATEXTURE_FLAG_STREAMED = 0x1,
    E_AVATEXTURE_FLAG_PLACEMENT = 0x2,
    E_AVATEXTURE_FLAG_TILED = 0x4,
    E_AVATEXTURE_FLAG_SRGB = 0x8,
    E_AVATEXTURE_FLAG_LOD_FROM_RENDER = 0x10,
    E_AVATEXTURE_FLAG_CUBE = 0x40,
    E_AVATEXTURE_FLAG_WATCH = 0x8000,
} AVATextureFlag;


typedef struct {
    char ident[4];
    uint8 version;
    uint8 platform;
    uint8 tag;
    uint8 resource_dimensions;
    DDSDXGIFormat format;
    uint16 width, height, depth;
    uint16 flags;
    uint8 mip_count;
    uint8 mip_resident;
    uint8 mip_cinematic;
    uint8 mip_bias;
    uint8 lod_group;
    uint8 pool;
    uint8 reserved0[2];
    uint32 reserved1;
    AVTXStream streams[8];
} AVTXHeader;

#pragma pack(pop)

void AVTXTexture_from_buffer(Buffer *buffer, Texture *texture);

#endif //APEXPREDATOR_AVTX_H
