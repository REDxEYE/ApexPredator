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

#include <array>
#include <cstddef>
#include <filesystem>
#include <cstring>


using namespace AVTX;


bool operator&(AVATextureFlag lhs, AVATextureFlag rhs) {
    return (static_cast<uint32>(lhs) & static_cast<uint32>(rhs)) != 0;
}

std::unique_ptr<Texture> AVTX::from_buffer(std::unique_ptr<IO::File> &&buffer, const uint64 hash, ApexArchiveManager &manager) {
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

    const auto load_streams = [&](const auto &streams) -> std::unique_ptr<Texture> {
        std::vector<uint8> compressed_data;
        if (header.flags & AVATextureFlag::STREAMED) {
            std::filesystem::path atx_path = {};
            int highest_mip_stream = -1;
            for (int i = 7; i >= 0; --i) {
                if (streams[i].size != 0) {
                    highest_mip_stream = i;
                    break;
                }
            }
            if (highest_mip_stream < 0) {
                goto BUILTIN_MIPS;
            }

            const auto path = find_asset_name(hash);
            if (!path.has_value() || path->empty()) {
                return nullptr;
            }

            const auto &stream = streams[highest_mip_stream];
            atx_path = *path;
            atx_path.replace_extension(std::format("atx{}", stream.source));
            auto atx_buffer = manager.get(asset_path_hash(atx_path.generic_string()));
            if (!atx_buffer) {
                GLog_Error("Expected ATX file not found for streamed AVTX texture: {}", atx_path.string());
                goto BUILTIN_MIPS;
            }

            const int64 largest_mip_size = Texture::calculate_mip_size(0, header.width, header.height, header.format);
            const int64 aligned_largest_mip_size = ALIGN_UP(largest_mip_size, stream.alignment);
            if (aligned_largest_mip_size != stream.size) {
                GLog_Error("Expected mip size does not match stream size, expected: {}, actual: {}",
                           aligned_largest_mip_size, stream.size);
                goto BUILTIN_MIPS;
            }
            atx_buffer->set_position(stream.offset);
            compressed_data.resize(stream.size);
            atx_buffer->read_exact(compressed_data);
            atx_buffer->close();
        }
        else {
        BUILTIN_MIPS:
            int resident_stream = -1;
            for (int i = 7; i >= 0; --i) {
                if (streams[i].source == 0 && streams[i].size != 0) {
                    resident_stream = i;
                    break;
                }
            }
            if (resident_stream < 0) {
                return nullptr;
            }
            const auto &stream = streams[resident_stream];
            buffer->set_position(stream.offset);
            compressed_data.resize(stream.size);
            buffer->read_exact(compressed_data);
        }

        return std::make_unique<Texture>(Texture::from_dxgi(header.format, compressed_data, header.width,
                                                            header.height, header.depth));
    };

    // Both layouts use version 1. The first resident payload begins immediately
    // after the stream table, so its offset distinguishes the table stride.
    if (header.streams[0].offset == sizeof(Header)) {
        return load_streams(header.streams);
    }
    constexpr auto stream_table_offset = offsetof(Header, streams);
    if (header.streams[0].offset == stream_table_offset + 8 * sizeof(SecondExtinctionTextureStream)) {
        buffer->set_position(stream_table_offset);
        const auto streams = buffer->read_pod<std::array<SecondExtinctionTextureStream, 8>>();
        return load_streams(streams);
    }
    GLog_Error("Unsupported AVTX stream table offset: {}", header.streams[0].offset);
    return nullptr;
}
