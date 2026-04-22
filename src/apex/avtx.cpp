// Created by RED on 30.09.2025.

#include "apex/avtx.h"

#include "apex/hashes.h"

#include "tinycpng/public/library.h"
#include "tinycpng/public/error_utils.h"
#include "tinycpng/public/file_utils.h"

#include "redscore/platform/logger.h"
#include "redscore/utils/common.h"
#include "tracy/Tracy.hpp"
#include "utils/hash_helper.h"

#include <filesystem>
#include <cstring>


using namespace AVTX;


bool operator&(AVATextureFlag lhs, AVATextureFlag rhs) {
    return (static_cast<uint32>(lhs) & static_cast<uint32>(rhs)) != 0;
}

std::unique_ptr<Texture> AVTX::from_buffer(std::unique_ptr<IO::File> &&buffer, const uint32 hash, ArchiveManager &manager) {
    ZoneScoped
    const auto header = buffer->read_pod<Header>();
    if (std::memcmp(header.ident, "AVTX", 4) != 0) {
        GLog_Error("Invalid AVTX texture format");
        return nullptr;
    }
    if (header.version != 1) {
        GLog_Error("Unsupported AVTX version: {}", header.version);
        return nullptr;
    }

    std::vector<uint8> compressed_data;
    if (header.flags & AVATextureFlag::STREAMED) {
        std::filesystem::path atx_path = {};
        const TextureStream *highest_mip_stream = nullptr;
        for (int i = 7; i >= 0; --i) {
            const TextureStream *stream = &header.streams[i];
            if (stream->size == 0) {
                continue;
            }
            highest_mip_stream = stream;
            break;
        }
        if (highest_mip_stream==nullptr) {
            goto BUILTIN_MIPS;
        }

        const auto path = find_name(hash);
        if (!path.has_value() || path->empty()) {
            return nullptr;
        }

        atx_path = *path;
        atx_path.replace_extension(std::format("atx{}", highest_mip_stream->source));
        auto atx_buffer = manager.get_file(hash_string(atx_path));
        if (!atx_buffer) {
            GLog_Error("Expected ATX file not found for streamed AVTX texture: {}", atx_path.string());
            goto BUILTIN_MIPS;
        }

        const int64 largest_mip_size = Texture::calculate_mip_size(0, header.width, header.height, header.format);
        const int64 aligned_largest_mip_size = ALIGN_UP(largest_mip_size, highest_mip_stream->alignment);
        if (aligned_largest_mip_size != highest_mip_stream->size) {
            GLog_Error("Expected mip size does not match stream size, expected: {}, actual: {}",
                       aligned_largest_mip_size,
                       highest_mip_stream->size);
            goto BUILTIN_MIPS;
        }
        atx_buffer->set_position(highest_mip_stream->offset);
        compressed_data.resize(highest_mip_stream->size);
        atx_buffer->read_exact(compressed_data);
        atx_buffer->close();
    }
    else {
    BUILTIN_MIPS:
        const std::optional non_streamed_stream = [&]() {
            for (int i = 7; i >= 0; --i) {
                if (header.streams[i].source == 0 && header.streams[i].size != 0) {
                    return &header.streams[i];
                }
            }
            return static_cast<const TextureStream *>(nullptr);
        }();
        if (!non_streamed_stream) {
            return nullptr;
        }
        buffer->set_position(non_streamed_stream.value()->offset);
        compressed_data.resize(non_streamed_stream.value()->size);
        buffer->read_exact(compressed_data);
    }

    return std::make_unique<Texture>(Texture::from_dxgi(header.format, compressed_data, header.width, header.height,
                                                        header.depth));
}

// void AVTXTexture_from_png(const String *png_path, const String *output_texture_name, const String *output_dir) {
//     FILE *file = fopen(String_cstr(png_path), "rb");
//     UserIO user_io = UserIO{.read_func = native_file_read, .write_func = native_file_write, .user_file = file};
//     PNGFile png = {};
//     png_read(&user_io, &png);
//
//     png_free(&png);
// }
