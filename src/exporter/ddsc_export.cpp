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
#include "utils/common.h"

Texture *convert_ddsc(const AppState *app_state, const uint32 hash) {
    CHECK_APP_STATE(app_state);
    MemoryBuffer mb = {};
    const ArchiveManager *archive_manager = &app_state->archive_manager;
    if (!ArchiveManager_get_file_by_hash(archive_manager, hash, &mb)) {
        GLog_Error("File not found");
        return NULL;
    }
    Texture* tex = AVTXTexture_from_buffer((Buffer*)&mb, hash, &app_state->archive_manager);
    mb.close(&mb);
    return tex;
}

void export_ddsc(const AppState *app_state, const uint32 hash, Buffer *mb) {
    Texture *tex = AVTXTexture_from_buffer((Buffer*)&mb, hash, &app_state->archive_manager);
    if (tex == NULL) {
        GLog_Error("Failed to convert AVTX texture with hash %08X", hash);
        return;
    }
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
    }
    else {
        String_format(&texture_export_path, "%s/texture_%08X", String_cstr(&app_state->export_path), hash);
    }

    Texture_save(tex, &texture_export_path);
    String_free(&texture_export_path);
    Texture_free(tex);
}
