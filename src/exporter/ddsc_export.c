// Created by RED on 12.01.2026.

#include "exporter/ddsc_export.h"

#include "apex/avtx.h"
#include "apex/hashes.h"
#include "platform/logger.h"
#include "platform/texture.h"
#include "utils/hash_helper.h"
#include "utils/path.h"

#include "utils/memory_profiling.h"
#include "tracy/TracyC.h"

Texture* convert_ddsc(AppState* app_state, const uint32 hash) {
    CHECK_APP_STATE(app_state);
    TracyCZoneN(ctx, "convert_ddsc", 1);
    MemoryBuffer mb = {0};
    ArchiveManager *archive_manager = &app_state->archive_manager;
    if (!ArchiveManager_get_file_by_hash(archive_manager, hash, &mb)) {
        GLog_Error("File not found");
        TracyCZoneEnd(ctx);
        return NULL;
    }
    AVTXHeader header;
    mb.read(&mb, &header, sizeof(header), NULL);

    if (strncmp(header.ident, "AVTX", 4) != 0) {
        GLog_Error("Invalid AVTX texture format");
        mb.close(&mb);
        return NULL;
    }
    if (header.version != 1) {
        GLog_Error("Unsupported AVTX version: %d", header.version);
        mb.close(&mb);
        return NULL;
    }
    Texture* texture = Texture_new();
    uint32 actual_body_size = 0;
    uint8 *compressed_data;
    if (header.flags & 1) {
        String atx_path = {0};
        const StringView path = find_name32_sv(hash);
        // highest mips are in atx<N> file
        for (int i = 5; i > 0; --i) {
            Path_remove_extension_sv(path, &atx_path);
            String_append_format(&atx_path, ".atx%i", i);
            if (ArchiveManager_has_file_by_hash(archive_manager, hash_string(&atx_path))) {
                break;
            }
        }
        MemoryBuffer atx_buffer = {0};
        ArchiveManager_get_file_by_hash(archive_manager, hash_string(&atx_path), &atx_buffer);
        if (atx_buffer.size==0) {
            Texture_free(texture);
            mb.close(&mb);
            TracyCZoneEnd(ctx);
            return NULL;
        }

        const int64 largest_mip_size = Texture_calculate_mip_size(0, header.width, header.height, header.format);
        atx_buffer.set_position(&atx_buffer, -largest_mip_size, BUFFER_ORIGIN_END);
        compressed_data = mp_malloc(largest_mip_size);
        atx_buffer.read(&atx_buffer, compressed_data, largest_mip_size, &actual_body_size);
        if (actual_body_size < largest_mip_size) {
            GLog_Error("Failed to read AVTX texture data, expected size: %u, actual size: %u", header.streams[0].size,
                   actual_body_size);
            mp_free(compressed_data);
            Texture_free(texture);
            mb.close(&mb);
            TracyCZoneEnd(ctx);
            return NULL;
        }
        atx_buffer.close(&atx_buffer);
        String_free(&atx_path);
    } else {
        mb.set_position(&mb, header.streams[0].offset, BUFFER_ORIGIN_START);
        compressed_data = mp_malloc(header.streams[0].size);
        mb.read(&mb, compressed_data, header.streams[0].size, &actual_body_size);
        if (actual_body_size != header.streams[0].size) {
            GLog_Error("Failed to read AVTX texture data, expected size: %u, actual size: %u", header.streams[0].size,
                   actual_body_size);
            mp_free(compressed_data);
            Texture_free(texture);
            mb.close(&mb);
            TracyCZoneEnd(ctx);
            return NULL;
        }
    }
    Texture_from_dxgi(texture, header.format, header.width, header.height, header.depth, compressed_data,
                      actual_body_size);
    mp_free(compressed_data);
    mb.close(&mb);
    TracyCZoneEnd(ctx);
    return texture;

}

void export_ddsc(AppState* app_state, uint32 hash, MemoryBuffer *mb) {
    CHECK_APP_STATE(app_state);
    String texture_export_path = {};


    const StringView path = find_name32_sv(hash);
    if (sv_is_not_null(path)) {
        String texture_without_ext = {};
        Path_remove_extension_sv(path, &texture_without_ext);
        Path_join(&texture_export_path, &app_state->export_path);
        Path_join(&texture_export_path, &texture_without_ext);
        String_free(&texture_without_ext);

        String tmp_check = {};
        String_copy_from(&tmp_check, &texture_export_path);
        String_append_cstr(&tmp_check, ".png");
        if (Path_exists(&tmp_check)) {
            String_free(&texture_export_path);
            return;
        }

    }else {
        String_format(&texture_export_path, "%s/texture_%08X", String_cstr(&app_state->export_path), hash);
    }
    Texture tex = {0};

    AVTXHeader header;
    mb->read(mb, &header, sizeof(header), NULL);
    if (strncmp(header.ident, "AVTX", 4) != 0) {
        GLog_Error("Invalid AVTX texture format");
        abort();
    }
    if (header.version != 1) {
        GLog_Error("Unsupported AVTX version: %d", header.version);
        abort();
    }
    uint32 actual_body_size = 0;
    uint8 *compressed_data;
    if (header.flags & 1) {
        String atx_path = {0};
        // highest mips are in atx<N> file
        for (int i = 5; i > 0; --i) {
            Path_remove_extension_sv(path, &atx_path);
            String_append_format(&atx_path, ".atx%i", i);
            if (ArchiveManager_has_file_by_hash(&app_state->archive_manager, hash_string(&atx_path))) {
                break;
            }
        }
        MemoryBuffer atx_buffer = {0};
        ArchiveManager_get_file_by_hash(&app_state->archive_manager, hash_string(&atx_path), &atx_buffer);

        const int64 largest_mip_size = Texture_calculate_mip_size(0, header.width, header.height, header.format);
        atx_buffer.set_position(&atx_buffer, -largest_mip_size, BUFFER_ORIGIN_END);
        compressed_data = mp_malloc(largest_mip_size);
        atx_buffer.read(&atx_buffer, compressed_data, largest_mip_size, &actual_body_size);
        if (actual_body_size < largest_mip_size) {
            GLog_Error("Failed to read AVTX texture data, expected size: %u, actual size: %u", header.streams[0].size,
                   actual_body_size);
            mp_free(compressed_data);
            abort();
        }
        atx_buffer.close(&atx_buffer);
        String_free(&atx_path);
    } else {
        mb->set_position(mb, header.streams[0].offset, BUFFER_ORIGIN_START);
        compressed_data = mp_malloc(header.streams[0].size);
        mb->read(mb, compressed_data, header.streams[0].size, &actual_body_size);
        if (actual_body_size != header.streams[0].size) {
            GLog_Error("Failed to read AVTX texture data, expected size: %u, actual size: %u", header.streams[0].size,
                   actual_body_size);
            mp_free(compressed_data);
            abort();
        }
    }
    Texture_from_dxgi(&tex, header.format, header.width, header.height, header.depth, compressed_data,
                      actual_body_size);
    mp_free(compressed_data);

    Texture_save(&tex, &texture_export_path);
    String_free(&texture_export_path);
    Texture_free(&tex);
}
