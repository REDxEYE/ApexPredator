// Created by RED on 12.01.2026.

#include "../../include/exporter/ddsc_export.h"

#include "apex/avtx.h"
#include "platform/logger.h"
#include "platform/texture.h"
#include "utils/hash_helper.h"
#include "utils/path.h"

String* export_ddsc_to_file(ArchiveManager *archive_manager, const String *path, const String *export_path) {
    if (path==NULL) {
        GLog_Error("Cannot export textures without name!");
        return NULL;
    }


    MemoryBuffer mb = {0};
    if (!ArchiveManager_get_file_by_hash(archive_manager, hash_string(path), &mb)) {
        GLog_Error("Texture file not found");
        return NULL;
    }

    String* output_file = String_new(64);

    String texture_without_ext = {};
    Path_remove_extension(path, &texture_without_ext);
    Path_join(output_file, export_path);
    Path_join(output_file, &texture_without_ext);
    String_free(&texture_without_ext);

    String tmp_check = {};
    String_copy_from(&tmp_check, output_file);
    String_append_cstr(&tmp_check, ".png");
    if (Path_exists(&tmp_check)) {
        String_free(output_file);
        return NULL;
    }

    return output_file;
}

Texture* convert_ddsc(ArchiveManager* archive_manager, const String *path) {
    MemoryBuffer mb = {0};
    if (!ArchiveManager_get_file(archive_manager, path, &mb)) {
        GLog_Error("File not found");
        return NULL;
    }
    AVTXHeader header;
    mb.read(&mb, &header, sizeof(header), NULL);

    if (strncmp(header.ident, "AVTX", 4) != 0) {
        GLog_Error("Invalid AVTX texture format");
        exit(1);
    }
    if (header.version != 1) {
        GLog_Error("Unsupported AVTX version: %d", header.version);
        exit(1);
    }
    Texture* texture = Texture_new();
    uint32 actual_body_size = 0;
    uint8 *compressed_data;
    if (header.flags & 1) {
        String atx_path = {0};
        // highest mips are in atx<N> file
        for (int i = 5; i > 0; --i) {
            Path_remove_extension(path, &atx_path);
            String_append_format(&atx_path, ".atx%i", i);
            if (ArchiveManager_has_file(archive_manager, &atx_path)) {
                break;
            }
        }
        MemoryBuffer atx_buffer = {0};
        ArchiveManager_get_file(archive_manager, &atx_path, &atx_buffer);
        if (atx_buffer.size==0) {
            Texture_free(texture);
            return NULL;
        }

        const int64 largest_mip_size = Texture_calculate_mip_size(0, header.width, header.height, header.format);
        atx_buffer.set_position(&atx_buffer, -largest_mip_size, BUFFER_ORIGIN_END);
        compressed_data = malloc(largest_mip_size);
        atx_buffer.read(&atx_buffer, compressed_data, largest_mip_size, &actual_body_size);
        if (actual_body_size < largest_mip_size) {
            GLog_Error("Failed to read AVTX texture data, expected size: %u, actual size: %u", header.body_size,
                   actual_body_size);
            free(compressed_data);
            Texture_free(texture);
            return NULL;
        }
        atx_buffer.close(&atx_buffer);
        String_free(&atx_path);
    } else {
        mb.set_position(&mb, header.header_size, BUFFER_ORIGIN_START);
        compressed_data = malloc(header.body_size);
        mb.read(&mb, compressed_data, header.body_size, &actual_body_size);
        if (actual_body_size != header.body_size) {
            GLog_Error("Failed to read AVTX texture data, expected size: %u, actual size: %u", header.body_size,
                   actual_body_size);
            free(compressed_data);
            Texture_free(texture);
            return NULL;
        }
    }
    Texture_from_dxgi(texture, header.format, header.width, header.height, header.depth, compressed_data,
                      actual_body_size);
    free(compressed_data);

    return texture;

}

void export_ddsc(ArchiveManager *archive_manager, uint32 hash, MemoryBuffer *mb,
                 const String *path, const String *export_path) {
    if (path==NULL) {
        GLog_Error("Cannot export textures without name!");
        return;
    }
    String texture_export_path = {};
    String texture_without_ext = {};
    Path_remove_extension(path, &texture_without_ext);
    Path_join(&texture_export_path, export_path);
    Path_join(&texture_export_path, &texture_without_ext);

    String tmp_check = {};
    String_copy_from(&tmp_check, &texture_export_path);
    String_append_cstr(&tmp_check, ".png");
    if (Path_exists(&tmp_check)) {
        String_free(&texture_export_path);
        String_free(&texture_without_ext);
        return;
    }
    Texture tex = {0};

    AVTXHeader header;
    mb->read(mb, &header, sizeof(header), NULL);
    if (strncmp(header.ident, "AVTX", 4) != 0) {
        GLog_Error("Invalid AVTX texture format");
        exit(1);
    }
    if (header.version != 1) {
        GLog_Error("Unsupported AVTX version: %d", header.version);
        exit(1);
    }
    uint32 actual_body_size = 0;
    uint8 *compressed_data;
    if (header.flags & 1) {
        String atx_path = {0};
        // highest mips are in atx<N> file
        for (int i = 5; i > 0; --i) {
            Path_remove_extension(path, &atx_path);
            String_append_format(&atx_path, ".atx%i", i);
            if (ArchiveManager_has_file(archive_manager, &atx_path)) {
                break;
            }
        }
        MemoryBuffer atx_buffer = {0};
        ArchiveManager_get_file(archive_manager, &atx_path, &atx_buffer);

        const int64 largest_mip_size = Texture_calculate_mip_size(0, header.width, header.height, header.format);
        atx_buffer.set_position(&atx_buffer, -largest_mip_size, BUFFER_ORIGIN_END);
        compressed_data = malloc(largest_mip_size);
        atx_buffer.read(&atx_buffer, compressed_data, largest_mip_size, &actual_body_size);
        if (actual_body_size < largest_mip_size) {
            GLog_Error("Failed to read AVTX texture data, expected size: %u, actual size: %u", header.body_size,
                   actual_body_size);
            free(compressed_data);
            exit(1);
        }
        atx_buffer.close(&atx_buffer);
        String_free(&atx_path);
    } else {
        mb->set_position(mb, header.header_size, BUFFER_ORIGIN_START);
        compressed_data = malloc(header.body_size);
        mb->read(mb, compressed_data, header.body_size, &actual_body_size);
        if (actual_body_size != header.body_size) {
            GLog_Error("Failed to read AVTX texture data, expected size: %u, actual size: %u", header.body_size,
                   actual_body_size);
            free(compressed_data);
            exit(1);
        }
    }
    Texture_from_dxgi(&tex, header.format, header.width, header.height, header.depth, compressed_data,
                      actual_body_size);
    free(compressed_data);

    Texture_save(&tex, &texture_export_path);
    String_free(&texture_export_path);
    String_free(&texture_without_ext);
    Texture_free(&tex);
}
