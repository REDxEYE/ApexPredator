// Created by RED on 16.02.2026.

#include "exporter/common_export.h"
#include "platform/app_state.h"
#include "platform/cli_parser.h"
#include "platform/logger.h"
#include "utils/common.h"
#include "utils/hash_helper.h"
#include "utils/path.h"

void raw_export(const AppState *app_state, const uint32 asset_hash) {
    String *asset_path = find_name32(asset_hash);
    if (asset_path == NULL) {
        asset_path = String_new(28);
        String_format(asset_path, "%08X.bin", asset_hash);
    }
    Path_normalize_posix(asset_path);

    String save_path = {0};
    String_copy_from(&save_path, &app_state->export_path);
    Path_join(&save_path, asset_path);
    Path_ensure_parent_dirs(&save_path);

    MemoryBuffer mb = {0};
    if (!ArchiveManager_get_file_by_hash(&app_state->archive_manager, asset_hash, &mb)) {
        GLog_Error("File not found: %г", asset_hash);
        String_free(asset_path);
        String_free(&save_path);
        return;
    }

    FILE *f = fopen(String_cstr(&save_path), "wb");
    if (f == NULL) {
        GLog_Error("Failed to open file for writing: %s", String_cstr(&save_path));
        mb.close(&mb);
        String_free(asset_path);
        String_free(&save_path);
        return;
    }
    fwrite(mb.data, 1, mb.size, f);
    fclose(f);
    mb.close(&mb);
    GLog_Info("File \"%u\" extracted to \"%s\"", asset_hash, String_cstr(&save_path));
    String_free(&save_path);
}

void normal_export(AppState *app_state, const uint32 asset_hash) {
    String *asset_path = find_name32(asset_hash);
    if (asset_path == NULL) {
        asset_path = String_new(28);
        String_format(asset_path, "%08X.bin", asset_hash);
    }
    Path_normalize_posix(asset_path);

    GLTFContext_init(&app_state->gltf_context, "root");
    Path_normalize_posix(asset_path);
    String save_path = {0};
    String_copy_from(&save_path, &app_state->export_path);
    Path_join(&save_path, asset_path);
    Path_replace_extension_inplace(&save_path, "gltf");
    GLTFContext_set_save_path(&app_state->gltf_context, &save_path);
    String_free(&save_path);

    export_file(app_state, asset_hash);

    GLTFContext_write_and_free(&app_state->gltf_context);
    String_free(asset_path);
}

void extract_handler(AppState *app_state, const CliResult *cli_res) {
    cli_get_bool(cli_res, "no_textures", &app_state->skip_textures);

    bool export_raw = false;
    cli_get_bool(cli_res, "raw", &export_raw);

    const char **file_paths = NULL;
    size_t file_path_count = 0;
    cli_get_array_string(cli_res, "paths", &file_paths, &file_path_count);
    for (int file_id = 0; file_id < file_path_count; ++file_id) {
        const char *file_path_cstr = file_paths[file_id];
        uint32 file_hash;
        if (is_hex(file_path_cstr)) {
            file_hash = parse_hex_u32(file_path_cstr);
        }
        else if (is_digits(file_path_cstr)) {
            file_hash = parse_digits_u32(file_path_cstr);
        }
        else {
            String skeleton_path_tmp = {0};
            String_from_cstr(&skeleton_path_tmp, file_path_cstr);
            Path_normalize_posix(&skeleton_path_tmp);
            file_hash = hash_string(&skeleton_path_tmp);
            String_free(&skeleton_path_tmp);
        }

        if (export_raw) {
            raw_export(app_state, file_hash);
        }
        else {
            normal_export(app_state, file_hash);
        }
    }
}
