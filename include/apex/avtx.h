// Created by RED on 30.09.2025.

#ifndef APEXPREDATOR_AVTX_H
#define APEXPREDATOR_AVTX_H
#include "int_def.h"
#include "platform/archive_manager.h"
#include "redscore/platform/texture.h"
#include "redscore/platform/file/file.h"

#define AVTX_MAGIC "AVTX"

namespace AVTX {
#pragma pack(push, 1)
    typedef struct {
        uint32 offset;
        uint32 size;
        uint16 alignment;
        uint8 tile_mode;
        uint8 source;
    } TextureStream;

    enum class AVATextureFlag: uint16 {
        STREAMED = 0x1,
        PLACEMENT = 0x2,
        TILED = 0x4,
        SRGB = 0x8,
        LOD_FROM_RENDER = 0x10,
        CUBE = 0x40,
        WATCH = 0x8000,
    };


    typedef struct {
        char ident[4];
        uint8 version;
        uint8 platform;
        uint8 tag;
        uint8 resource_dimensions;
        DDSDXGIFormat format;
        uint16 width, height, depth;
        AVATextureFlag flags;
        uint8 mip_count;
        uint8 mip_resident;
        uint8 mip_cinematic;
        uint8 mip_bias;
        uint8 lod_group;
        uint8 pool;
        uint8 reserved0[2];
        uint32 reserved1;
        TextureStream streams[8];

    } Header;

#pragma pack(pop)

    std::unique_ptr<Texture> from_buffer(std::unique_ptr<IO::File> &&buffer, uint32 hash, ArchiveManager& manager);

    // void AVTXTexture_from_png(const String* png_path, const String* output_texture_name, const String *output_dir);
}
#endif //APEXPREDATOR_AVTX_H
