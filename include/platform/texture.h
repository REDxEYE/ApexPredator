// Created by RED on 30.09.2025.

#ifndef APEXPREDATOR_TEXTURE_H
#define APEXPREDATOR_TEXTURE_H
#include "int_def.h"
#include "utils/string.h"

typedef enum  {
    DXGI_FORMAT_R16G16B16A16_TYPELESS       =  9,
    DXGI_FORMAT_R16G16B16A16_FLOAT          = 10,
    DXGI_FORMAT_R16G16B16A16_UNORM          = 11,
    DXGI_FORMAT_R16G16B16A16_UINT           = 12,
    DXGI_FORMAT_R16G16B16A16_SNORM          = 13,
    DXGI_FORMAT_R16G16B16A16_SINT           = 14,

    DXGI_FORMAT_R8G8B8A8_TYPELESS           = 27,
    DXGI_FORMAT_R8G8B8A8_UNORM              = 28,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB         = 29,
    DXGI_FORMAT_R8G8B8A8_UINT               = 30,
    DXGI_FORMAT_R8G8B8A8_SNORM              = 31,
    DXGI_FORMAT_R8G8B8A8_SINT               = 32,

    DXGI_FORMAT_BC1_TYPELESS                = 70,
    DXGI_FORMAT_BC1_UNORM                   = 71,
    DXGI_FORMAT_BC1_UNORM_SRGB              = 72,
    DXGI_FORMAT_BC2_TYPELESS                = 73,
    DXGI_FORMAT_BC2_UNORM                   = 74,
    DXGI_FORMAT_BC2_UNORM_SRGB              = 75,
    DXGI_FORMAT_BC3_TYPELESS                = 76,
    DXGI_FORMAT_BC3_UNORM                   = 77,
    DXGI_FORMAT_BC3_UNORM_SRGB              = 78,
    DXGI_FORMAT_BC4_TYPELESS                = 79,
    DXGI_FORMAT_BC4_UNORM                   = 80,
    DXGI_FORMAT_BC4_SNORM                   = 81,
    DXGI_FORMAT_BC5_TYPELESS                = 82,
    DXGI_FORMAT_BC5_UNORM                   = 83,
    DXGI_FORMAT_BC5_SNORM                   = 84,
    DXGI_FORMAT_B5G6R5_UNORM                = 85,
    DXGI_FORMAT_B8G8R8A8_UNORM              = 87,
    DXGI_FORMAT_B8G8R8X8_UNORM              = 88,
    DXGI_FORMAT_B8G8R8A8_TYPELESS           = 90,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB         = 91,
    DXGI_FORMAT_B8G8R8X8_TYPELESS           = 92,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB         = 93,
    DXGI_FORMAT_FORCE_UINT32         = 0x7FFFFFFF,
}DDSDXGIFormat;

typedef struct {
    int32 width, height, depth;
    uint32 bpc:5;
    uint32 channel_count:5;
    uint32 is_float;
    uint8* data;
    uint32 data_size;
}Texture;

void Texture_init(Texture* texture, int32 width, int32 height, int32 depth, uint16 bpc, uint16 channel_count);

void Texture_from_dxgi(Texture* texture, DDSDXGIFormat format, int32 width, int32 height, int32 depth, const uint8* input, uint32 input_size);

void Texture_save(const Texture* texture, const String* path_without_ext);

void Texture_free(Texture* texture);

uint32 Texture_calculate_mip_size(uint32 mip, uint32 width, uint32 height, DDSDXGIFormat format);
#endif //APEXPREDATOR_TEXTURE_H