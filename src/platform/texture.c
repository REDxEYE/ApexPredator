// Created by RED on 30.09.2025.

#include "platform/texture.h"
#include "bcdec.h"
#include <assert.h>

#include "utils/path.h"
#include "utils/stb_image_write.h"

void Texture_init(Texture *texture, int32 width, int32 height, int32 depth, uint16 bpc, uint16 channel_count) {
    texture->width = width;
    texture->height = height;
    texture->depth = depth;
    texture->bpc = bpc;
    texture->channel_count = channel_count;
    texture->data = NULL;
    texture->data_size = 0;
}

void Texture__decode_texture(Texture *texture, const uint8 *input, uint32 input_size, DDSDXGIFormat format) {
    const uint32 expected_size = texture->width * texture->height * texture->depth * texture->channel_count * texture->
                                 bpc;
    if (texture->data == NULL || texture->data_size != expected_size) {
        if (texture->data) {
            free(texture->data);
        }
        texture->data = malloc(expected_size);
        texture->data_size = expected_size;
    }
    switch (format) {
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: {
            if (input_size < expected_size) {
                printf("[ERROR]: Unexpected input size: %u, expected: %u\n", input_size, expected_size);
                assert(false && "Unexpected input size");
                exit(1);
            }
            memcpy(texture->data, input, expected_size);
            break;
        }
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB: {
            const uint32 block_size = 4;
            const uint32 blocks_wide = (texture->width + 3) / 4;
            const uint32 blocks_high = (texture->height + 3) / 4;
            const uint32 expected_compressed_size = blocks_wide * blocks_high * block_size;
            if (input_size < expected_compressed_size) {
                printf("[ERROR]: Unexpected input size: %u, expected: %u\n", input_size, expected_compressed_size);
                assert(false && "Unexpected input size");
                exit(1);
            }
            const uint32 block_count = blocks_wide * blocks_high;
            for (int i = 0; i < block_count; ++i) {
                bcdec_bc1(input + i * 8,
                          texture->data + (i % blocks_wide) * 4 * 4 + (i / blocks_wide) * 4 * texture->width * 4,
                          texture->width * 4);
            }
            break;
        }
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB: {
            const uint32 block_size = 8;
            const uint32 blocks_wide = (texture->width + 3) / 4;
            const uint32 blocks_high = (texture->height + 3) / 4;
            const uint32 expected_compressed_size = blocks_wide * blocks_high * block_size;
            if (input_size < expected_compressed_size) {
                printf("[ERROR]: Unexpected input size: %u, expected: %u\n", input_size, expected_compressed_size);
                assert(false && "Unexpected input size");
                exit(1);
            }
            const uint32 block_count = blocks_wide * blocks_high;
            for (int i = 0; i < block_count; ++i) {
                bcdec_bc2(input + i * 16,
                          texture->data + (i % blocks_wide) * 4 * 4 + (i / blocks_wide) * 4 * texture->width * 4,
                          texture->width * 4);
            }
            break;
        }
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB: {
            const uint32 block_size = 8;
            const uint32 blocks_wide = (texture->width + 3) / 4;
            const uint32 blocks_high = (texture->height + 3) / 4;
            const uint32 expected_compressed_size = blocks_wide * blocks_high * block_size;
            if (input_size < expected_compressed_size) {
                printf("[ERROR]: Unexpected input size: %u, expected: %u\n", input_size, expected_compressed_size);
                assert(false && "Unexpected input size");
                exit(1);
            }
            const uint32 block_count = blocks_wide * blocks_high;
            for (int i = 0; i < block_count; ++i) {
                bcdec_bc3(input + i * 16,
                          texture->data + (i % blocks_wide) * 4 * 4 + (i / blocks_wide) * 4 * texture->width * 4,
                          texture->width * 4);
            }
            break;
        }
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM: {
            const uint32 block_size = 4;
            const uint32 blocks_wide = (texture->width + 3) / 4;
            const uint32 blocks_high = (texture->height + 3) / 4;
            const uint32 expected_compressed_size = blocks_wide * blocks_high * block_size;
            if (input_size < expected_compressed_size) {
                printf("[ERROR]: Unexpected input size: %u, expected: %u\n", input_size, expected_compressed_size);
                assert(false && "Unexpected input size");
                exit(1);
            }
            const uint32 block_count = blocks_wide * blocks_high;
            for (int i = 0; i < block_count; ++i) {
                bcdec_bc4(input + i * 8, texture->data + (i % blocks_wide) * 4 + (i / blocks_wide) * 4 * texture->width,
                          texture->width);
            }
            break;
        }
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM: {
            const uint32 block_size = 8;
            const uint32 blocks_wide = (texture->width + 3) / 4;
            const uint32 blocks_high = (texture->height + 3) / 4;
            const uint32 expected_compressed_size = blocks_wide * blocks_high * block_size;
            if (input_size < expected_compressed_size) {
                printf("[ERROR]: Unexpected input size: %u, expected: %u\n", input_size, expected_compressed_size);
                assert(false && "Unexpected input size");
                exit(1);
            }
            const uint32 block_count = blocks_wide * blocks_high;
            for (int i = 0; i < block_count; ++i) {
                bcdec_bc5(input + i * 16,
                          texture->data + (i % blocks_wide) * 4 * 2 + (i / blocks_wide) * 4 * texture->width * 2,
                          texture->width * 2);
            }
            break;
        }
        default: {
            printf("[ERROR]: Unsupported DXGI format: %d\n", format);
            assert(false && "Unsupported DXGI format");
            exit(1);
        }
    }
}

void Texture_from_dxgi(Texture *texture, DDSDXGIFormat format, int32 width, int32 height, int32 depth,
                       const uint8 *input, uint32 input_size) {
    texture->width = width;
    texture->height = height;
    texture->depth = depth;
    texture->data = NULL;
    uint32 target_channels = 0;
    uint32 target_bpc = 0;
    switch (format) {
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
            target_channels = 4;
            target_bpc = 2;
            break;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            target_channels = 4;
            target_bpc = 1;
            break;
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            target_channels = 1;
            target_bpc = 1;
            break;
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
            target_channels = 2;
            target_bpc = 1;
            break;
        case DXGI_FORMAT_B5G6R5_UNORM:
            target_channels = 3;
            target_bpc = 1;
            break;
        default:
            printf("[ERROR]: Unsupported DXGI format: %d\n", format);
            assert(false && "Unsupported DXGI format");
            exit(1);
    }
    texture->bpc = target_bpc;
    texture->channel_count = target_channels;
    const uint32 expected_size = width * height * depth * target_channels * target_bpc;
    texture->data = malloc(expected_size);
    texture->data_size = expected_size;
    Texture__decode_texture(texture, input, input_size, format);
}

void Texture_save(const Texture *texture, const String *path_without_ext) {
    if (texture->data == NULL || texture->data_size == 0) {
        printf("[WARNING]: Texture data is empty, skipping save\n");
        return;
    }
    String path_png = {};
    String_copy_from(&path_png, path_without_ext);
    Path_ensure_parent_dirs(&path_png);
    if (texture->bpc >= 2 && texture->is_float) {
        String_append_cstr(&path_png, ".hdr");
        stbi_write_hdr(String_data(&path_png), texture->width, texture->height, texture->channel_count,
                       (float32 *) texture->data);
    } else {
        int32 comp = texture->channel_count;
        if (comp != 1 && comp != 2 && comp != 3 && comp != 4) {
            printf("[ERROR]: Unsupported channel count: %d\n", texture->channel_count);
            assert(false && "Unsupported channel count");
            exit(1);
        }
        int32 stride = texture->width * texture->channel_count * texture->bpc;
        if (texture->bpc == 1) {
            String_append_cstr(&path_png, ".png");
            stbi_write_png(String_data(&path_png), texture->width, texture->height, comp, texture->data, stride);
        } else {
            printf("[ERROR]: Unsupported bpc: %d\n", texture->bpc);
            assert(false && "Unsupported bpc");
            exit(1);
        }
        String_free(&path_png);
    }
}

void Texture_free(Texture *texture) {
    if (texture->data) {
        free(texture->data);
        texture->data = NULL;
    }
    texture->data_size = 0;
}

uint32 Texture_calculate_mip_size(uint32 mip, uint32 width, uint32 height, DDSDXGIFormat format) {
    uint32 mip_width = width >> mip;
    uint32 mip_height = height >> mip;
    if (mip_width == 0) mip_width = 1;
    if (mip_height == 0) mip_height = 1;

    switch (format) {
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return mip_width * mip_height * 4 * 2;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return mip_width * mip_height * 4 * 1;
        case DXGI_FORMAT_B5G6R5_UNORM:
            return mip_width * mip_height * 3 * 1;
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB: {
            uint32 blocks_wide = (mip_width + 3) / 4;
            uint32 blocks_high = (mip_height + 3) / 4;
            return blocks_wide * blocks_high * 8;
        }
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB: {
            uint32 blocks_wide = (mip_width + 3) / 4;
            uint32 blocks_high = (mip_height + 3) / 4;
            return blocks_wide * blocks_high * 8;
        }
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB: {
            uint32 blocks_wide = (mip_width + 3) / 4;
            uint32 blocks_high = (mip_height + 3) / 4;
            return blocks_wide * blocks_high * 8;
        }
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM: {
            uint32 blocks_wide = (mip_width + 3) / 4;
            uint32 blocks_high = (mip_height + 3) / 4;
            return blocks_wide * blocks_high * 4;
        }
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM: {
            uint32 blocks_wide = (mip_width + 3) / 4;
            uint32 blocks_high = (mip_height + 3) / 4;
            return blocks_wide * blocks_high * 8;
        }
        default:
            printf("[ERROR]: Unsupported DXGI format: %d\n", format);
            assert(false && "Unsupported DXGI format");
            exit(1);
    }
}
