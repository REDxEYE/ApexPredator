// Created by RED on 30.09.2025.

#include "apex/avtx.h"

#include "tinycpng/public/library.h"
#include "tinycpng/public/error_utils.h"
#include "tinycpng/public/file_utils.h"
#include "tinycpng/public/mytypes.h"

#include "platform/logger.h"
#include "utils/common.h"
#include "utils/hash_helper.h"
#include "utils/path.h"

Texture *AVTXTexture_from_buffer(Buffer *buffer, const uint32 hash, const ArchiveManager *manager) {
    TracyCZoneN(ctx, "AVTXTexture_from_buffer", 1);
    AVTXHeader header;
    buffer->read(buffer, &header, sizeof(header), NULL);
    if (strncmp(header.ident, "AVTX", 4) != 0) {
        GLog_Error("Invalid AVTX texture format");
        TracyCZoneEnd(ctx);
        return NULL;
    }
    if (header.version != 1) {
        GLog_Error("Unsupported AVTX version: %d", header.version);
        TracyCZoneEnd(ctx);
        return NULL;
    }

    uint32 actual_body_size = 0;
    uint8 *compressed_data;
    Texture *texture = Texture_new();
    if (header.flags & E_AVATEXTURE_FLAG_STREAMED) {
        String atx_path = {};
        const AVTXStream *highest_mip_stream = NULL;
        for (int i = 7; i > 0; --i) {
            const AVTXStream *stream = &header.streams[i];
            if (stream->size == 0) {
                continue;
            }
            highest_mip_stream = stream;
            break;
        }

        const StringView path = find_name32_sv(hash);
        Path_remove_extension_sv(path, &atx_path);
        String_append_format(&atx_path, ".atx%i", highest_mip_stream->source);
        if (!ArchiveManager_has_file_by_hash(manager, hash_string(&atx_path))) {
            GLog_Error("Expected ATX file not found for streamed AVTX texture: %s", String_cstr(&atx_path));
            String_free(&atx_path);
            goto BUILTIN_MIPS;
        }
        MemoryBuffer atx_buffer = {};
        ArchiveManager_get_file_by_hash(manager, hash_string(&atx_path), &atx_buffer);


        const int64 largest_mip_size = Texture_calculate_mip_size(0, header.width, header.height, header.format);
        const int64 aligned_largest_mip_size = ALIGN_UP(largest_mip_size, highest_mip_stream->alignment);
        if (aligned_largest_mip_size != highest_mip_stream->size) {
            GLog_Error("Expected mip size does not match stream size, expected: %u, actual: %u",
                       aligned_largest_mip_size,
                       highest_mip_stream->size);
            atx_buffer.close(&atx_buffer);
            String_free(&atx_path);
            goto BUILTIN_MIPS;
        }
        atx_buffer.set_position(&atx_buffer, highest_mip_stream->offset, BUFFER_ORIGIN_START);
        compressed_data = (uint8*)mp_malloc(highest_mip_stream->size);
        atx_buffer.read(&atx_buffer, compressed_data, highest_mip_stream->size, &actual_body_size);

        atx_buffer.close(&atx_buffer);
        String_free(&atx_path);

        if (actual_body_size < highest_mip_stream->size) {
            GLog_Error("Failed to read AVTX texture data, expected size: %u, actual size: %u", header.streams[0].size,
                       actual_body_size);
            goto CLEANUP;
        }
    }
    else {
    BUILTIN_MIPS:
        buffer->set_position(buffer, header.streams[0].offset, BUFFER_ORIGIN_START);
        compressed_data = (uint8*)mp_malloc(header.streams[0].size);
        buffer->read(buffer, compressed_data, header.streams[0].size, &actual_body_size);
        if (actual_body_size != header.streams[0].size) {
            GLog_Error("Failed to read AVTX texture data, expected size: %u, actual size: %u", header.streams[0].size,
                       actual_body_size);
            goto CLEANUP;
        }
    }

    Texture_from_dxgi(texture, header.format, header.width, header.height, header.depth,
                      compressed_data, actual_body_size);

    TracyCZoneEnd(ctx);
    return texture;

CLEANUP:
    if (compressed_data != NULL)
        mp_free(compressed_data);
    buffer->close(buffer);

    TracyCZoneEnd(ctx);
    return NULL;
}

void AVTXTexture_from_png(const String* png_path, const String* output_texture_name, const String *output_dir) {

    FILE* file = fopen(String_cstr(png_path), "rb");
    UserIO user_io = UserIO{.read_func = native_file_read, .write_func = native_file_write, .user_file = file};
    PNGFile png = {};
    png_read(&user_io, &png);

    png_free(&png);
}
