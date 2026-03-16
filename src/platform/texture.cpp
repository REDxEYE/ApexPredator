// Created by RED on 30.09.2025.

#include "platform/texture.h"
#include "bcdec.h"
#include <cassert>
#include <cstring>
#include <filesystem>

#include "library.h"
#include "tracy/Tracy.hpp"
#include "platform/logger.h"
#include "utils/stb_image_write.h"


float32 float16_to_float32(uint16 v) {
    uint32 sign = (v & 0x8000) << 16;
    uint32 exponent = (v & 0x7C00) >> 10;
    uint32 mantissa = v & 0x03FF;

    if (exponent == 0) {
        if (mantissa == 0) {
            // Zero
            return *(float32 *) &sign;
        }
        else {
            // Subnormal number
            while ((mantissa & 0x0400) == 0) {
                mantissa <<= 1;
                exponent--;
            }
            exponent++;
            mantissa &= ~0x0400;
        }
    }
    else if (exponent == 31) {
        uint32 tmp = (sign | 0x7F800000 | (mantissa << 13));
        // Inf or NaN
        return *(float32 *) &tmp;
    }

    exponent = exponent + (127 - 15);
    mantissa = mantissa << 13;

    uint32 result = sign | (exponent << 23) | mantissa;
    return *(float32 *) &result;
}


uint32 Texture::calculate_mip_size(const uint32 mip, const uint32 width, const uint32 height,
                                   const DDSDXGIFormat format) {
    uint32 mip_width = width >> mip;
    uint32 mip_height = height >> mip;
    if (mip_width == 0) mip_width = 1;
    if (mip_height == 0) mip_height = 1;

    switch (format) {
        case DDSDXGIFormat::DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DDSDXGIFormat::DXGI_FORMAT_R16G16B16A16_UNORM:
        case DDSDXGIFormat::DXGI_FORMAT_R16G16B16A16_UINT:
        case DDSDXGIFormat::DXGI_FORMAT_R16G16B16A16_SNORM:
        case DDSDXGIFormat::DXGI_FORMAT_R16G16B16A16_SINT:
            return mip_width * mip_height * 4 * 2;
        case DDSDXGIFormat::DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DDSDXGIFormat::DXGI_FORMAT_R8G8B8A8_UNORM:
        case DDSDXGIFormat::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DDSDXGIFormat::DXGI_FORMAT_R8G8B8A8_UINT:
        case DDSDXGIFormat::DXGI_FORMAT_R8G8B8A8_SNORM:
        case DDSDXGIFormat::DXGI_FORMAT_R8G8B8A8_SINT:
        case DDSDXGIFormat::DXGI_FORMAT_B8G8R8A8_UNORM:
        case DDSDXGIFormat::DXGI_FORMAT_B8G8R8X8_UNORM:
        case DDSDXGIFormat::DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DDSDXGIFormat::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DDSDXGIFormat::DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DDSDXGIFormat::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return mip_width * mip_height * 4 * 1;
        case DDSDXGIFormat::DXGI_FORMAT_B5G6R5_UNORM:
            return mip_width * mip_height * 3 * 1;
        case DDSDXGIFormat::DXGI_FORMAT_BC1_TYPELESS:
        case DDSDXGIFormat::DXGI_FORMAT_BC1_UNORM:
        case DDSDXGIFormat::DXGI_FORMAT_BC1_UNORM_SRGB: {
            uint32 blocks_wide = (mip_width + 3) / 4;
            uint32 blocks_high = (mip_height + 3) / 4;
            return blocks_wide * blocks_high * 8;
        }
        case DDSDXGIFormat::DXGI_FORMAT_BC2_TYPELESS:
        case DDSDXGIFormat::DXGI_FORMAT_BC2_UNORM:
        case DDSDXGIFormat::DXGI_FORMAT_BC2_UNORM_SRGB: {
            uint32 blocks_wide = (mip_width + 3) / 4;
            uint32 blocks_high = (mip_height + 3) / 4;
            return blocks_wide * blocks_high * 16;
        }
        case DDSDXGIFormat::DXGI_FORMAT_BC3_TYPELESS:
        case DDSDXGIFormat::DXGI_FORMAT_BC3_UNORM:
        case DDSDXGIFormat::DXGI_FORMAT_BC3_UNORM_SRGB: {
            uint32 blocks_wide = (mip_width + 3) / 4;
            uint32 blocks_high = (mip_height + 3) / 4;
            return blocks_wide * blocks_high * 16;
        }
        case DDSDXGIFormat::DXGI_FORMAT_BC4_TYPELESS:
        case DDSDXGIFormat::DXGI_FORMAT_BC4_UNORM:
        case DDSDXGIFormat::DXGI_FORMAT_BC4_SNORM: {
            uint32 blocks_wide = (mip_width + 3) / 4;
            uint32 blocks_high = (mip_height + 3) / 4;
            return blocks_wide * blocks_high * 8;
        }
        case DDSDXGIFormat::DXGI_FORMAT_BC5_TYPELESS:
        case DDSDXGIFormat::DXGI_FORMAT_BC5_UNORM:
        case DDSDXGIFormat::DXGI_FORMAT_BC5_SNORM: {
            uint32 blocks_wide = (mip_width + 3) / 4;
            uint32 blocks_high = (mip_height + 3) / 4;
            return blocks_wide * blocks_high * 16;
        }
        default:
            GLog_Error("Unsupported DXGI format: {}", format);
            throw std::runtime_error("Unsupported DXGI format");
    }
}

Texture Texture::from_dxgi(DDSDXGIFormat format, std::span<const uint8> data, int32 width, int32 height, int16 depth) {
    ZoneScoped
    if (data.size() == 0) {
        throw std::runtime_error("Empty input data");
    }
    std::vector<uint8> decoded_data;
    uint8 bpc, channels;
    bool is_float = false;
    switch (format) {
        case DDSDXGIFormat::DXGI_FORMAT_R16G16B16A16_TYPELESS:
            throw std::runtime_error("Unsupported format: DXGI_FORMAT_R16G16B16A16_TYPELESS");
        case DDSDXGIFormat::DXGI_FORMAT_R16G16B16A16_FLOAT: {
            is_float = true;
            bpc = 2;
            channels = 4;
            decoded_data.resize(width * height * 4 * sizeof(uint16));
            std::memcpy(decoded_data.data(), data.data(), decoded_data.size());
            break;
        }
        case DDSDXGIFormat::DXGI_FORMAT_R16G16B16A16_UNORM: {
            bpc = 2;
            channels = 4;
            decoded_data.resize(width * height * 4 * sizeof(uint16));
            std::memcpy(decoded_data.data(), data.data(), decoded_data.size());
            break;
        }
        case DDSDXGIFormat::DXGI_FORMAT_R16G16B16A16_UINT: {
            throw std::runtime_error("Unsupported format: DXGI_FORMAT_R16G16B16A16_UINT");
        }
        case DDSDXGIFormat::DXGI_FORMAT_R16G16B16A16_SNORM: {
            throw std::runtime_error("Unsupported format: DXGI_FORMAT_R16G16B16A16_SNORM");
        }
        case DDSDXGIFormat::DXGI_FORMAT_R16G16B16A16_SINT: {
            throw std::runtime_error("Unsupported format: DXGI_FORMAT_R16G16B16A16_SINT");
        }
        case DDSDXGIFormat::DXGI_FORMAT_R8G8B8A8_TYPELESS: {
            throw std::runtime_error("Unsupported format: DXGI_FORMAT_R8G8B8A8_TYPELESS");
        }
        case DDSDXGIFormat::DXGI_FORMAT_R8G8B8A8_UNORM: {
            bpc = 1;
            channels = 4;
            decoded_data.resize(width * height * 4 * sizeof(uint8));
            std::memcpy(decoded_data.data(), data.data(), decoded_data.size());
            break;
        }
        case DDSDXGIFormat::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: {
            bpc = 1;
            channels = 4;
            decoded_data.resize(width * height * 4 * sizeof(uint8));
            std::memcpy(decoded_data.data(), data.data(), decoded_data.size());
            break;
        }
        case DDSDXGIFormat::DXGI_FORMAT_R8G8B8A8_UINT: {
            throw std::runtime_error("Unsupported format: DXGI_FORMAT_R8G8B8A8_UINT");
        }
        case DDSDXGIFormat::DXGI_FORMAT_R8G8B8A8_SNORM: {
            throw std::runtime_error("Unsupported format: DXGI_FORMAT_R8G8B8A8_SNORM");
        }
        case DDSDXGIFormat::DXGI_FORMAT_R8G8B8A8_SINT: {
            throw std::runtime_error("Unsupported format: DXGI_FORMAT_R8G8B8A8_SINT");
        }
        case DDSDXGIFormat::DXGI_FORMAT_R16G16_UNORM: {
            bpc = 2;
            channels = 2;
            decoded_data.resize(width * height * 2 * sizeof(uint16));
            std::memcpy(decoded_data.data(), data.data(), decoded_data.size());
            break;
        }
        case DDSDXGIFormat::DXGI_FORMAT_R16_UNORM: {
            bpc = 2;
            channels = 1;
            decoded_data.resize(width * height * sizeof(uint16));
            std::memcpy(decoded_data.data(), data.data(), decoded_data.size());
            break;
        }
        case DDSDXGIFormat::DXGI_FORMAT_R8_UNORM: {
            bpc = 1;
            channels = 1;
            decoded_data.resize(width * height * sizeof(uint8));
            std::memcpy(decoded_data.data(), data.data(), decoded_data.size());
            break;
        }
        case DDSDXGIFormat::DXGI_FORMAT_BC1_TYPELESS:
        case DDSDXGIFormat::DXGI_FORMAT_BC1_UNORM:
        case DDSDXGIFormat::DXGI_FORMAT_BC1_UNORM_SRGB: {
            bpc = 1;
            channels = 4;
            constexpr uint32 block_size = 4;
            const uint32 blocks_wide = (width + 3) / 4;
            const uint32 blocks_high = (height + 3) / 4;
            const uint32 expected_compressed_size = blocks_wide * blocks_high * block_size;
            if (data.size() < expected_compressed_size) {
                GLog_Error("Unexpected input size: %u, expected: %u", data.size(), expected_compressed_size);
                throw std::runtime_error("Unexpected input size");
            }
            const uint8 *input = data.data();
            decoded_data.resize(width * height * 4);
            uint8 *output = decoded_data.data();
            const uint32 block_count = blocks_wide * blocks_high;
            for (int i = 0; i < block_count; ++i) {
                bcdec_bc1(input + i * 8,
                          output + (i % blocks_wide) * 4 * 4 + (i / blocks_wide) * 4 * width * 4,
                          width * 4);
            }
            break;
        }
        case DDSDXGIFormat::DXGI_FORMAT_BC2_TYPELESS:
        case DDSDXGIFormat::DXGI_FORMAT_BC2_UNORM:
        case DDSDXGIFormat::DXGI_FORMAT_BC2_UNORM_SRGB: {
            bpc = 1;
            channels = 4;
            constexpr uint32 block_size = 8;
            const uint32 blocks_wide = (width + 3) / 4;
            const uint32 blocks_high = (height + 3) / 4;
            const uint32 expected_compressed_size = blocks_wide * blocks_high * block_size;
            if (data.size() < expected_compressed_size) {
                GLog_Error("Unexpected input size: %u, expected: %u", data.size(), expected_compressed_size);
                throw std::runtime_error("Unexpected input size");
            }
            const uint8 *input = data.data();
            decoded_data.resize(width * height * 4);
            uint8 *output = decoded_data.data();
            const uint32 block_count = blocks_wide * blocks_high;
            for (int i = 0; i < block_count; ++i) {
                bcdec_bc2(input + i * 16,
                          output + (i % blocks_wide) * 4 * 4 + (i / blocks_wide) * 4 * width * 4,
                          width * 4);
            }
            break;
        }
        case DDSDXGIFormat::DXGI_FORMAT_BC3_TYPELESS:
        case DDSDXGIFormat::DXGI_FORMAT_BC3_UNORM:
        case DDSDXGIFormat::DXGI_FORMAT_BC3_UNORM_SRGB: {
            bpc = 1;
            channels = 4;
            constexpr uint32 block_size = 16;
            const uint32 blocks_wide = (width + 3) / 4;
            const uint32 blocks_high = (height + 3) / 4;
            const uint32 expected_compressed_size = blocks_wide * blocks_high * block_size;
            if (data.size() < expected_compressed_size) {
                GLog_Error("Unexpected input size: %u, expected: %u", data.size(), expected_compressed_size);
                throw std::runtime_error("Unexpected input size");
            }
            const uint8 *input = data.data();
            decoded_data.resize(width * height * 4);
            uint8 *output = decoded_data.data();
            const uint32 block_count = blocks_wide * blocks_high;
            for (int i = 0; i < block_count; ++i) {
                bcdec_bc3(input + i * 16,
                          output + (i % blocks_wide) * 4 * 4 + (i / blocks_wide) * 4 * width * 4,
                          width * 4);
            }
            break;
        }
        case DDSDXGIFormat::DXGI_FORMAT_BC4_TYPELESS:
        case DDSDXGIFormat::DXGI_FORMAT_BC4_UNORM:
        case DDSDXGIFormat::DXGI_FORMAT_BC4_SNORM: {
            bpc = 1;
            channels = 1;
            constexpr uint32 block_size = 4;
            const uint32 blocks_wide = (width + 3) / 4;
            const uint32 blocks_high = (height + 3) / 4;
            const uint32 expected_compressed_size = blocks_wide * blocks_high * block_size;
            if (data.size() < expected_compressed_size) {
                GLog_Error("Unexpected input size: %u, expected: %u", data.size(), expected_compressed_size);
                throw std::runtime_error("Unexpected input size");
            }
            const uint8 *input = data.data();
            decoded_data.resize(width * height);
            uint8 *output = decoded_data.data();
            const uint32 block_count = blocks_wide * blocks_high;
            for (int i = 0; i < block_count; ++i) {
                bcdec_bc4(input + i * 8, output + (i % blocks_wide) * 4 + (i / blocks_wide) * 4 * width,
                          width);
            }
            break;
        }
        case DDSDXGIFormat::DXGI_FORMAT_BC5_TYPELESS:
        case DDSDXGIFormat::DXGI_FORMAT_BC5_UNORM:
        case DDSDXGIFormat::DXGI_FORMAT_BC5_SNORM: {
            bpc = 1;
            channels = 2;
            constexpr uint32 block_size = 8;
            const uint32 blocks_wide = (width + 3) / 4;
            const uint32 blocks_high = (height + 3) / 4;
            const uint32 expected_compressed_size = blocks_wide * blocks_high * block_size;
            if (data.size() < expected_compressed_size) {
                GLog_Error("Unexpected input size: %u, expected: %u", data.size(), expected_compressed_size);
                throw std::runtime_error("Unexpected input size");
            }
            const uint8 *input = data.data();
            decoded_data.resize(width * height * 2);
            uint8 *output = decoded_data.data();
            const uint32 block_count = blocks_wide * blocks_high;
            for (int i = 0; i < block_count; ++i) {
                bcdec_bc5(input + i * 16,
                          output + (i % blocks_wide) * 4 * 2 + (i / blocks_wide) * 4 * width * 2,
                          width * 2);
            }
            break;
        }
        case DDSDXGIFormat::DXGI_FORMAT_BC7_UNORM: {
            bpc = 1;
            channels = 4;
            constexpr uint32 block_size = 16;
            const uint32 blocks_wide = (width + 3) / 4;
            const uint32 blocks_high = (height + 3) / 4;
            const uint32 expected_compressed_size = blocks_wide * blocks_high * block_size;
            if (data.size() < expected_compressed_size) {
                GLog_Error("Unexpected input size: %u, expected: %u", data.size(), expected_compressed_size);
                throw std::runtime_error("Unexpected input size");
            }
            const uint8 *input = data.data();
            decoded_data.resize(width * height * 4);
            uint8 *output = decoded_data.data();
            const uint32 block_count = blocks_wide * blocks_high;
            for (int i = 0; i < block_count; ++i) {
                bcdec_bc7(input + i * 16,
                          output + (i % blocks_wide) * 4 * 4 + (i / blocks_wide) * 4 * width * 4,
                          width * 4);
            }
            break;
        }

        case DDSDXGIFormat::DXGI_FORMAT_B5G6R5_UNORM: {
            bpc = 1;
            channels = 3;
            const auto *input = reinterpret_cast<const uint16 *>(data.data());
            decoded_data.resize(width * height * 3);
            for (int i = 0; i < width * height; ++i) {
                uint16 pixel = input[i];
                uint8 r = (pixel >> 11) & 0x1F;
                uint8 g = (pixel >> 5) & 0x3F;
                uint8 b = pixel & 0x1F;
                decoded_data[i * 3 + 0] = (r << 3) | (r >> 2);
                decoded_data[i * 3 + 1] = (g << 2) | (g >> 4);
                decoded_data[i * 3 + 2] = (b << 3) | (b >> 2);
            }

            break;
        }
        case DDSDXGIFormat::DXGI_FORMAT_B8G8R8A8_UNORM:
        case DDSDXGIFormat::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DDSDXGIFormat::DXGI_FORMAT_B8G8R8A8_TYPELESS: {
            bpc = 1;
            channels = 4;
            decoded_data.resize(width * height * 4);
            const auto *input = reinterpret_cast<const uint8 *>(data.data());
            for (int i = 0; i < width * height; ++i) {
                uint8 b = input[i * 4 + 0];
                uint8 g = input[i * 4 + 1];
                uint8 r = input[i * 4 + 2];
                uint8 a = input[i * 4 + 3];
                decoded_data[i * 4 + 0] = r;
                decoded_data[i * 4 + 1] = g;
                decoded_data[i * 4 + 2] = b;
                decoded_data[i * 4 + 3] = a;
            }
            break;
        }
        case DDSDXGIFormat::DXGI_FORMAT_B8G8R8X8_UNORM:
        case DDSDXGIFormat::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DDSDXGIFormat::DXGI_FORMAT_B8G8R8X8_TYPELESS: {
            bpc = 1;
            channels = 4;
            decoded_data.resize(width * height * 3);
            const auto *input = reinterpret_cast<const uint8 *>(data.data());
            for (int i = 0; i < width * height; ++i) {
                uint8 b = input[i * 4 + 0];
                uint8 g = input[i * 4 + 1];
                uint8 r = input[i * 4 + 2];
                decoded_data[i * 3 + 0] = r;
                decoded_data[i * 3 + 1] = g;
                decoded_data[i * 3 + 2] = b;
            }
            break;
        }
        case DDSDXGIFormat::DXGI_FORMAT_CUSTOM_R8G8B8_UNORM: {
            bpc = 1;
            channels = 3;
            std::memcpy(decoded_data.data(), data.data(), decoded_data.size());
            break;
        }
        case DDSDXGIFormat::DXGI_FORMAT_FORCE_UINT32: {
            throw std::runtime_error("Unsupported format: DXGI_FORMAT_FORCE_UINT32");
        }
    }

    Texture texture(width, height, depth, bpc, channels, is_float, std::move(decoded_data));
    return std::move(texture);
}

void Texture::save(const std::filesystem::path &path_without_ext) const {
    ZoneScoped
    if (m_is_float) {
        std::vector<float32> float_data(m_width * m_height * m_depth * m_channel_count);
        if (m_bpc == 2) {
            const auto *src = reinterpret_cast<const uint16 *>(m_data.data());
            float32 *dst = float_data.data();
            const int pixel_count = m_width * m_height * m_depth * m_channel_count;
            for (int i = 0; i < pixel_count; ++i) {
                dst[i] = float16_to_float32(src[i]);
            }
        }
        else if (m_bpc == 4) {
            const auto *src = reinterpret_cast<const float32 *>(m_data.data());
            const int pixel_count = m_width * m_height * m_depth * m_channel_count;
            std::copy_n(src, pixel_count, float_data.data());
        }
        else {
            GLog_Error("Unsupported bpc for float texture: %d", m_bpc);
            throw std::runtime_error(std::format("Unsupported bpc({}) for float texture", m_bpc));
        }
        std::filesystem::path output_path = path_without_ext;
        output_path += ".hdr";
        stbi_write_hdr(output_path.string().c_str(), m_width, m_height, m_channel_count, float_data.data());
    }
    else {
        if (m_bpc != 1) {
            GLog_Error("Unsupported bpc for non-float texture: %d", m_bpc);
            throw std::runtime_error(std::format("Unsupported bpc({}) for non-float texture", m_bpc));
        }
        if (m_channel_count < 1 || m_channel_count > 4) {
            GLog_Error("Unsupported channel count for non-float texture: %d", m_channel_count);
            throw std::runtime_error(
                std::format("Unsupported channel count({}) for non-float texture", m_channel_count));
        }
        std::filesystem::path output_path = path_without_ext;
        output_path += ".png";

        PNGWriteConfig config = {};
        PNGWriteConfig_default(&config);
        // if (texture->width <= 1024 && texture->height <= 1024) {
        //     config.scan_palette = true;
        // }
        PNGFile file = {};
        png_from_data(m_data.data(), m_data.size(), m_width, m_height, m_channel_count, 8, &file);

        FILE *file_handle = fopen(output_path.string().c_str(), "wb");
        UserIO io = {.read_func = native_file_read, .write_func = native_file_write, .user_file = file_handle};
        png_write(&io, &config, &file);

        png_free(&file);
        fclose(file_handle);
    }
}

std::vector<uint8> Texture::save_to_memory(MemoryFormat format) const {
    ZoneScoped
    if (format == MemoryFormat::PNG) {
        if (m_bpc != 1) {
            GLog_Error("Unsupported bpc for PNG output: %d", m_bpc);
            throw std::runtime_error(std::format("Unsupported bpc({}) for PNG output", m_bpc));
        }
        if (m_is_float) {
            GLog_Error("Cannot write float texture to PNG format");
            throw std::runtime_error("Cannot write float texture to PNG format");
        }
        if (m_channel_count < 1 || m_channel_count > 4) {
            GLog_Error("Unsupported channel count for PNG output: %d", m_channel_count);
            throw std::runtime_error(std::format("Unsupported channel count({}) for PNG output", m_channel_count));
        }

        std::vector<uint8> png_data;
        PNGWriteConfig config = {};
        PNGWriteConfig_default(&config);
        PNGFile file = {};
        png_from_data(m_data.data(), m_data.size(), m_width, m_height, m_channel_count, 8, &file);
        MemoryFile memory_file = {};
        UserIO io = {.read_func = memory_file_read, .write_func = memory_file_write, .user_file = &memory_file};

        png_write(&io, &config, &file);
        png_free(&file);

        png_data.resize(memory_file.size);
        std::copy_n(memory_file.data, memory_file.size, png_data.data());
        MemoryFile_free(&memory_file);

        return png_data;
    }
    GLog_Error("Unsupported memory format: {}", format);
    throw std::runtime_error(std::format("Unsupported memory format: {}", static_cast<int>(format)));
}
