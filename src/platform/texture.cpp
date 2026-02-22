// Created by RED on 30.09.2025.

#include "platform/texture.h"
#include "bcdec.h"
#include <assert.h>

#include "platform/logger.h"
#include "utils/memory_profiling.h"
#include "utils/path.h"
#include "utils/stb_image_write.h"
#include "tinycpng/public/library.h"
#include "tracy/TracyC.h"

Texture *Texture_new() {
    Texture *texture = (Texture *)mp_malloc(sizeof(Texture));
    memset(texture, 0, sizeof(Texture));
    texture->heap_allocated = 1;
    return texture;
}

void Texture_init(Texture *texture, const int32 width, const int32 height, const int32 depth,
                  const uint16 bpc, const uint16 channel_count) {
    texture->width = width;
    texture->height = height;
    texture->depth = depth;
    texture->bpc = bpc;
    texture->channel_count = channel_count;
    texture->data = NULL;
    texture->data_size = 0;
}

void Texture__decode_texture(Texture *texture, const uint8 *input, const uint32 input_size,
                             const DDSDXGIFormat format) {
    TracyCZoneN(ctx, "Texture__decode_texture", 1);
    const uint32 expected_size = texture->width * texture->height * texture->depth * texture->channel_count * texture->
                                 bpc;
    if (texture->data) {
        mp_free(texture->data);
    }
    texture->data = (uint8*)mp_malloc(expected_size);
    texture->data_size = expected_size;

    switch (format) {
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            texture->is_float=true;
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
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: {
            if (input_size < expected_size) {
                GLog_Error("Unexpected input size: %u, expected: %u", input_size, expected_size);
                TracyCZoneEnd(ctx);
                assert(false && "Unexpected input size");
                abort();
            }
            memcpy(texture->data, input, expected_size);
            break;
        }
        case DXGI_FORMAT_R16G16_UNORM: {
            // Input is u16, but we store is as u8, so expected size is half from what we actually need
            if (input_size < expected_size*2) {
                GLog_Error("Unexpected input size: %u, expected: %u", input_size, expected_size/2);
                TracyCZoneEnd(ctx);
                assert(false && "Unexpected input size");
                abort();
            }
            const int pixel_count = texture->width * texture->height;
            for (int i = 0; i < pixel_count; ++i) {
                const uint16 *pixel_value = &((uint16 *) input)[i * 2];
                // Convert in float accurate mode (uint8_t)((u16 * 255u + 0x8000u) >> 16);
                texture->data[i * 2 + 0] = (uint8) ((pixel_value[0] * 255u + 0x8000u) >> 16);
                texture->data[i * 2 + 1] = (uint8) ((pixel_value[0] * 255u + 0x8000u) >> 16);


            }
            break;
        }

        case DXGI_FORMAT_R16_UNORM: {
            // Input is u16, but we store is as u8, so expected size is half from what we actually need
            if (input_size < expected_size*2) {
                GLog_Error("Unexpected input size: %u, expected: %u", input_size, expected_size/2);
                TracyCZoneEnd(ctx);
                assert(false && "Unexpected input size");
                abort();
            }
            for (int i = 0; i < texture->width * texture->height; ++i) {
                const uint16 pixel_value = ((uint16 *) input)[i];
                texture->data[i] = (uint8) ((pixel_value * 255u + 0x8000u) >> 16);
            }
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
                GLog_Error("Unexpected input size: %u, expected: %u", input_size, expected_compressed_size);
                TracyCZoneEnd(ctx);
                assert(false && "Unexpected input size");
                abort();
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
                GLog_Error("Unexpected input size: %u, expected: %u", input_size, expected_compressed_size);
                TracyCZoneEnd(ctx);
                assert(false && "Unexpected input size");
                abort();
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
            const uint32 block_size = 16;
            const uint32 blocks_wide = (texture->width + 3) / 4;
            const uint32 blocks_high = (texture->height + 3) / 4;
            const uint32 expected_compressed_size = blocks_wide * blocks_high * block_size;
            if (input_size < expected_compressed_size) {
                GLog_Error("Unexpected input size: %u, expected: %u", input_size, expected_compressed_size);
                TracyCZoneEnd(ctx);
                assert(false && "Unexpected input size");
                abort();
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
                GLog_Error("Unexpected input size: %u, expected: %u", input_size, expected_compressed_size);
                TracyCZoneEnd(ctx);
                assert(false && "Unexpected input size");
                abort();
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
                GLog_Error("Unexpected input size: %u, expected: %u", input_size, expected_compressed_size);
                TracyCZoneEnd(ctx);
                assert(false && "Unexpected input size");
                abort();
            }
            const uint32 block_count = blocks_wide * blocks_high;
            for (int i = 0; i < block_count; ++i) {
                bcdec_bc5(input + i * 16,
                          texture->data + (i % blocks_wide) * 4 * 2 + (i / blocks_wide) * 4 * texture->width * 2,
                          texture->width * 2);
            }
            break;
        }
        case DXGI_FORMAT_BC7_UNORM: {
            const uint32 block_size = 16;
            const uint32 blocks_wide = (texture->width + 3) / 4;
            const uint32 blocks_high = (texture->height + 3) / 4;
            const uint32 expected_compressed_size = blocks_wide * blocks_high * block_size;
            if (input_size < expected_compressed_size) {
                GLog_Error("Unexpected input size: %u, expected: %u", input_size, expected_compressed_size);
                TracyCZoneEnd(ctx);
                assert(false && "Unexpected input size");
                abort();
            }
            const uint32 block_count = blocks_wide * blocks_high;
            for (int i = 0; i < block_count; ++i) {
                bcdec_bc7(input + i * 16,
                          texture->data + (i % blocks_wide) * 4 * 4 + (i / blocks_wide) * 4 * texture->width * 4,
                          texture->width * 4);
            }
            break;
        }

        default: {
            GLog_Error("Unsupported DXGI format: %d", format);
            TracyCZoneEnd(ctx);
            assert(false && "Unsupported DXGI format");
            abort();
        }
    }
    TracyCZoneEnd(ctx);
}

void Texture_from_dxgi(Texture *texture, const DDSDXGIFormat format, const int32 width, const int32 height,
                       const int32 depth,
                       const uint8 *input, const uint32 input_size) {
    TracyCZoneN(ctx, "Texture_from_dxgi", 1);
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
        case DXGI_FORMAT_R16G16_UNORM:
            target_channels = 2;
            target_bpc = 1; // Compress into R8G8_UNORM
            break;
        case DXGI_FORMAT_R16_UNORM:
            target_channels = 1;
            target_bpc = 1; // Compress into R8_UNORM
            break;
        case DXGI_FORMAT_R8_UNORM:
            target_channels = 1;
            target_bpc = 1;
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
        case DXGI_FORMAT_BC7_UNORM:
            target_channels = 4;
            target_bpc = 1;
            break;
        case DXGI_FORMAT_B5G6R5_UNORM:
            target_channels = 3;
            target_bpc = 1;
            break;
        case DXGI_FORMAT_CUSTOM_R8G8B8_UNORM:
            target_channels = 3;
            target_bpc = 1;
            break;
        default:
            GLog_Error("Unsupported DXGI format: %d", format);
            assert(false && "Unsupported DXGI format");
            abort();
    }
    texture->bpc = target_bpc;
    texture->channel_count = target_channels;
    // const uint32 expected_size = width * height * depth * target_channels * target_bpc;
    // texture->data = mp_malloc(expected_size);
    // texture->data_size = expected_size;

    Texture__decode_texture(texture, input, input_size, format);
    TracyCZoneEnd(ctx);
}

float32 float16_to_float32(uint16 v) {
    uint32 sign = (v & 0x8000) << 16;
    uint32 exponent = (v & 0x7C00) >> 10;
    uint32 mantissa = v & 0x03FF;

    if (exponent == 0) {
        if (mantissa == 0) {
            // Zero
            return *(float32 *)&sign;
        } else {
            // Subnormal number
            while ((mantissa & 0x0400) == 0) {
                mantissa <<= 1;
                exponent--;
            }
            exponent++;
            mantissa &= ~0x0400;
        }
    } else if (exponent == 31) {
        uint32 tmp = (sign | 0x7F800000 | (mantissa << 13));
        // Inf or NaN
        return *(float32 *)&tmp;
    }

    exponent = exponent + (127 - 15);
    mantissa = mantissa << 13;

    uint32 result = sign | (exponent << 23) | mantissa;
    return *(float32 *)&result;
}

void Texture_save(const Texture *texture, const String *path_without_ext) {
    TracyCZoneN(ctx, "Texture_save", 1);
    if (texture->data == NULL || texture->data_size == 0) {
        GLog_Warning("Texture data is empty, skipping save");
        TracyCZoneEnd(ctx);
        return;
    }
    String path_png = {};
    String_copy_from(&path_png, path_without_ext);
    Path_ensure_parent_dirs(&path_png);
    if (texture->bpc >= 2 && texture->is_float) {
        String_append_cstr(&path_png, ".hdr");
        // Expand float16 to float32
        float* float_image = (float*)mp_malloc(texture->width * texture->height * texture->channel_count * sizeof(float));
        const uint16 *src = (const uint16 *) texture->data;
        float *dst = float_image;
        const int pixel_count = texture->width * texture->height;
        for (int i = 0; i < pixel_count * texture->channel_count; ++i) {
            dst[i] = float16_to_float32(src[i]);
        }


        stbi_write_hdr(String_cstr(&path_png), texture->width, texture->height, texture->channel_count,
                       float_image);
        mp_free(float_image);
    } else {
        int32 comp = texture->channel_count;
        if (comp != 1 && comp != 2 && comp != 3 && comp != 4) {
            GLog_Error("Unsupported channel count: %d", texture->channel_count);
            TracyCZoneEnd(ctx);
            assert(false && "Unsupported channel count");
            abort();
        }
        if (texture->bpc == 1) {
            String_append_cstr(&path_png, ".png");

            PNGWriteConfig config = {};
            PNGWriteConfig_default(&config);
            // if (texture->width <= 1024 && texture->height <= 1024) {
            //     config.scan_palette = true;
            // }
            PNGFile file = {};
            TracyCZoneN(ctx2, "png_from_data", 1);
            png_from_data(texture->data, texture->data_size, texture->width, texture->height, comp, 8, &file);
            TracyCZoneEnd(ctx2);

            FILE *file_handle = fopen(String_cstr(&path_png), "wb");
            UserIO io = {.read_func = native_file_read, .write_func = native_file_write, .user_file = file_handle};
            TracyCZoneN(ctx3, "png_write", 1);
            png_write(&io, &config, &file);
            TracyCZoneEnd(ctx3);
            png_free(&file);
            fclose(file_handle);
        } else {
            GLog_Error("Unsupported bpc: %d", texture->bpc);
            TracyCZoneEnd(ctx);
            assert(false && "Unsupported bpc");
            abort();
        }
        String_free(&path_png);
    }
    TracyCZoneEnd(ctx);
}

void *Texture_write_png_to_memory(const Texture *texture, uint32 *channel_count, size_t *out_size) {
    TracyCZoneN(ctx, "Texture_write_png_to_memory", 1);
    if (out_size == NULL) {
        GLog_Error("out_size pointer is NULL");
        TracyCZoneEnd(ctx);
        assert(false && "out_size pointer is NULL");
        return NULL;
    }
    if (texture->data == NULL || texture->data_size == 0) {
        GLog_Warning("Texture data is empty, cannot write to memory");
        TracyCZoneEnd(ctx);
        return NULL;
    }
    if (texture->bpc != 1) {
        GLog_Error("Unsupported bpc for PNG output: %d", texture->bpc);
        TracyCZoneEnd(ctx);
        assert(false && "Unsupported bpc for PNG output");
        return NULL;
    }
    if (texture->is_float) {
        GLog_Error("Cannot write float texture to PNG format");
        TracyCZoneEnd(ctx);
        assert(false && "Cannot write float texture to PNG format");
        return NULL;
    }

    const int32 comp = (int32) texture->channel_count;
    if (comp < 1 || comp > 4) {
        GLog_Error("Unsupported channel count for PNG output: %d", texture->channel_count);
        TracyCZoneEnd(ctx);
        assert(false && "Unsupported channel count for PNG output");
        return NULL;
    }
    uint8 *png_data = NULL;
    PNGWriteConfig config = {};
    PNGWriteConfig_default(&config);
    PNGFile file = {};
    TracyCZoneN(ctx2, "png_from_data", 1);
    png_from_data(texture->data, texture->data_size, texture->width, texture->height, comp, 8, &file);
    TracyCZoneEnd(ctx2);
    MemoryFile memory_file = {};
    UserIO io = {.read_func = memory_file_read, .write_func = memory_file_write, .user_file = &memory_file};

    TracyCZoneN(ctx3, "png_write", 1);
    png_write(&io, &config, &file);
    TracyCZoneEnd(ctx3);
    png_free(&file);

    *out_size = memory_file.size;
    if (channel_count != NULL) {
        *channel_count = comp;
    }
    png_data = (uint8*)mp_malloc(memory_file.size);
    memcpy(png_data, memory_file.data, memory_file.size);
    MemoryFile_free(&memory_file);

    TracyCZoneEnd(ctx);
    return png_data;
}

void Texture_free(Texture *texture) {
    if (texture->data) {
        mp_free(texture->data);
        texture->data = NULL;
    }
    texture->data_size = 0;

    if (texture->heap_allocated) {
        mp_free(texture);
    }
}

uint32 Texture_calculate_mip_size(const uint32 mip, const uint32 width, const uint32 height,
                                  const DDSDXGIFormat format) {
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
            return blocks_wide * blocks_high * 16;
        }
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB: {
            uint32 blocks_wide = (mip_width + 3) / 4;
            uint32 blocks_high = (mip_height + 3) / 4;
            return blocks_wide * blocks_high * 16;
        }
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM: {
            uint32 blocks_wide = (mip_width + 3) / 4;
            uint32 blocks_high = (mip_height + 3) / 4;
            return blocks_wide * blocks_high * 8;
        }
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM: {
            uint32 blocks_wide = (mip_width + 3) / 4;
            uint32 blocks_high = (mip_height + 3) / 4;
            return blocks_wide * blocks_high * 16;
        }
        default:
            GLog_Error("Unsupported DXGI format: %d", format);
            assert(false && "Unsupported DXGI format");
            abort();
    }
}
