// Created by RED on 30.09.2025.

#ifndef APEXPREDATOR_TEXTURE_H
#define APEXPREDATOR_TEXTURE_H
#include <filesystem>
#include <format>
#include <vector>
#include <span>

#include "int_def.h"

enum class DDSDXGIFormat:uint32 {
    DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
    DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
    DXGI_FORMAT_R16G16B16A16_UNORM = 11,
    DXGI_FORMAT_R16G16B16A16_UINT = 12,
    DXGI_FORMAT_R16G16B16A16_SNORM = 13,
    DXGI_FORMAT_R16G16B16A16_SINT = 14,
    DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
    DXGI_FORMAT_R8G8B8A8_UNORM = 28,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
    DXGI_FORMAT_R8G8B8A8_UINT = 30,
    DXGI_FORMAT_R8G8B8A8_SNORM = 31,
    DXGI_FORMAT_R8G8B8A8_SINT = 32,
    DXGI_FORMAT_R16G16_UNORM = 35,
    DXGI_FORMAT_R16_UNORM = 56,
    DXGI_FORMAT_R8_UNORM = 61,
    DXGI_FORMAT_BC1_TYPELESS = 70,
    DXGI_FORMAT_BC1_UNORM = 71,
    DXGI_FORMAT_BC1_UNORM_SRGB = 72,
    DXGI_FORMAT_BC2_TYPELESS = 73,
    DXGI_FORMAT_BC2_UNORM = 74,
    DXGI_FORMAT_BC2_UNORM_SRGB = 75,
    DXGI_FORMAT_BC3_TYPELESS = 76,
    DXGI_FORMAT_BC3_UNORM = 77,
    DXGI_FORMAT_BC3_UNORM_SRGB = 78,
    DXGI_FORMAT_BC4_TYPELESS = 79,
    DXGI_FORMAT_BC4_UNORM = 80,
    DXGI_FORMAT_BC4_SNORM = 81,
    DXGI_FORMAT_BC5_TYPELESS = 82,
    DXGI_FORMAT_BC5_UNORM = 83,
    DXGI_FORMAT_BC5_SNORM = 84,
    DXGI_FORMAT_B5G6R5_UNORM = 85,
    DXGI_FORMAT_B8G8R8A8_UNORM = 87,
    DXGI_FORMAT_B8G8R8X8_UNORM = 88,
    DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
    DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
    DXGI_FORMAT_BC7_UNORM = 98,
    DXGI_FORMAT_CUSTOM_R8G8B8_UNORM = 10001,
    DXGI_FORMAT_FORCE_UINT32 = 0x7FFFFFFF,
};

template<>
struct std::formatter<DDSDXGIFormat>:
        std::formatter<std::string_view> {
    auto format(const DDSDXGIFormat value, std::format_context &ctx) const {
        std::string_view name;
        switch (value) {
            case DDSDXGIFormat::DXGI_FORMAT_R16G16B16A16_TYPELESS:
                name = "DXGI_FORMAT_R16G16B16A16_TYPELESS";
                break;
            case DDSDXGIFormat::DXGI_FORMAT_R16G16B16A16_FLOAT:
                name = "DXGI_FORMAT_R16G16B16A16_FLOAT";
                break;
            case DDSDXGIFormat::DXGI_FORMAT_R16G16B16A16_UNORM:
                name = "DXGI_FORMAT_R16G16B16A16_UNORM";
                break;
            case DDSDXGIFormat::DXGI_FORMAT_R8G8B8A8_TYPELESS:
                name = "DXGI_FORMAT_R8G8B8A8_TYPELESS";
                break;
            case DDSDXGIFormat::DXGI_FORMAT_R8G8B8A8_UNORM:
                name = "DXGI_FORMAT_R8G8B8A8_UNORM";
                break;
            case DDSDXGIFormat::DXGI_FORMAT_BC1_TYPELESS:
                name = "DXGI_FORMAT_BC1_TYPELESS";
                break;
            case DDSDXGIFormat::DXGI_FORMAT_BC1_UNORM:
                name = "DXGI_FORMAT_BC1_UNORM";
                break;
            case DDSDXGIFormat::DXGI_FORMAT_BC1_UNORM_SRGB:
                name = "DXGI_FORMAT_BC1_UNORM_SRGB";
                break;
            default:
                name = "Unknown format";
        }
        return std::formatter<std::string_view>::format(name, ctx);
    }
};


enum class MemoryFormat {
    PNG = 0,
};

template<>
struct std::formatter<MemoryFormat>: std::formatter<std::string_view> {
    auto format(const MemoryFormat value, std::format_context &ctx) const {
        std::string_view name;
        switch (value) {
            case MemoryFormat::PNG:
                name = "PNG";
                break;
            default:
                name = "Unknown format";
        }
        return std::formatter<std::string_view>::format(name, ctx);
    }
};

class Texture {
public:
    Texture(const int32 width, const int32 height, const int32 depth, const uint8 bpc,
            const uint8 channel_count, const bool is_float) : m_width(width), m_height(height), m_depth(depth), m_bpc(bpc),
                                                        m_channel_count(channel_count), m_is_float(is_float) {
        m_data.reserve(width * height * depth * channel_count * bpc);
    }

    Texture(const int32 width, const int32 height, const int32 depth, const uint8 bpc, const uint8 channel_count,
            const bool is_float, std::vector<uint8> &&data) : Texture(width, height, depth, bpc, channel_count,
                                                                      is_float) {
        m_data = std::move(data);
    }

    static uint32 calculate_mip_size(uint32 mip, uint32 width, uint32 height, DDSDXGIFormat format);

    static Texture from_dxgi(DDSDXGIFormat format, std::span<const uint8> data, int32 width, int32 height, int16 depth);

    void save(const std::filesystem::path &path_without_ext) const;

    [[nodiscard]] std::vector<uint8> save_to_memory(MemoryFormat format) const;

    // Getters
    [[nodiscard]] int32 width() const { return m_width; }
    [[nodiscard]] int32 height() const { return m_height; }
    [[nodiscard]] int16 depth() const { return m_depth; }
    [[nodiscard]] uint8 bpc() const { return m_bpc; }
    [[nodiscard]] uint8 channel_count() const { return m_channel_count; }
    [[nodiscard]] bool is_float() const { return m_is_float; }
    [[nodiscard]] const std::vector<uint8> &data() const { return m_data; }
    std::vector<uint8> &data() { return m_data; }

private:
    int32 m_width, m_height;
    int16 m_depth;
    uint8 m_bpc;
    uint8 m_channel_count;
    uint32 m_is_float;
    std::vector<uint8> m_data;
};


#endif //APEXPREDATOR_TEXTURE_H
