// Created by RED on 12.01.2026.

#include "exporter/common_export.h"

#include "apex/avtx.h"
#include "apex/hashes.h"
#include "apex/rtpc.h"
#include "apex/sarc.h"
#include "apex/aaf/aaf.h"
#include "apex/adf/adf.h"
#include "exporter/adf_export.h"
#include "exporter/ddsc_export.h"
#include "exporter/rtpc_export.h"
#include "exporter/fmod_export.h"
#include "exporter/havok_export.h"
#include "utils/path.h"
#include "platform/logger.h"
#include "utils/simple_fileio.h"

#define MVK_MAGIC "\x1A\x45\xDF\xA3"

void mount_archive(const ArchiveManager *manager, const uint32 hash) {
    if (ArchiveManager_mounted(manager, hash)) {
        return;
    }
    MemoryBuffer mb = {};
    StringView file_name = find_name32_sv(hash);
    if (!ArchiveManager_get_file_by_hash(manager, hash, &mb)) {
        if (sv_is_not_null(file_name)) {
            GLog_Error("File \"%s\" not found", StringView_cstr(file_name));
        }
        else {
            GLog_Error("File 0x%08X not found", hash);
        }
        Buffer_close((Buffer *) &mb);
        return;
    }
    if (memcmp(mb.data, AAF_MAGIC, 4) == 0) {
        if (sv_is_not_null(file_name)) {
            GLog_Info("Mounting AAF archive \"%s\"", StringView_cstr(file_name));
        }
        else {
            GLog_Info("Mounting AAF archive with hash 0x%08X", hash);
        }
        AAFArchive aaf_archive = {};
        AAFArchive_from_buffer(&aaf_archive, (Buffer *) &mb);
        MemoryBuffer *section_buffer = MemoryBuffer_new();
        if (!AAFArchive_get_data(&aaf_archive, section_buffer)) {
            GLog_Error("Failed to get AAF section 0");
            Buffer_close((Buffer *) &mb);
            return;
        }

        SArchive *sarc = SArchive_new((Buffer *) section_buffer, hash); // sarc is now owner of buffer
        ArchiveManager_add(manager, (Archive *) sarc);
        AAFArchive_free(&aaf_archive);
        Buffer_close((Buffer *) &mb);
    }
}


GL_ID export_file(AppState *app_state, const uint32 hash) {
    CHECK_GLTF_STATE(&app_state->gltf_context);
    const StringView path = find_name32_sv(hash);
    GLog_Info("Exporting file: %s", sv_is_not_null(path) ? StringView_cstr(path) : "unknown");
    MemoryBuffer mb = {};
    if (!ArchiveManager_get_file_by_hash(&app_state->archive_manager, hash, &mb)) {
        if (sv_is_not_null(path)) {
            GLog_Error("File \"%s\" not found", StringView_cstr(path));
            return INVALID_GL_ID;
        }
        GLog_Error("File 0x%08X not found", hash);
        return INVALID_GL_ID;
    }
    GL_ID output_node_id = INVALID_GL_ID;

    if (memcmp(mb.data, ADF_MAGIC, 4) == 0) {
        output_node_id = export_adf_file_from_buffer(app_state, hash, &mb);
    }
    else if (memcmp(mb.data, AVTX_MAGIC, 4) == 0) {
        export_ddsc(app_state, hash, (Buffer *) &mb);
    }
    else if (memcmp(mb.data, RIFF_MAGIC, 4) == 0) {
        export_fmod_bank(app_state, hash, &mb);
    }
    else if (memcmp(mb.data, RTPC_MAGIC, 4) == 0) {
        output_node_id = export_rtpc(app_state, (Buffer *) &mb, hash);
    }
    else if (memcmp(mb.data + 4, HAVOK_MAGIC, 4) == 0) {
        output_node_id = export_havok_file(app_state, (Buffer*)&mb, path);
    }
    else if (memcmp(mb.data, MVK_MAGIC, 4) == 0) {
        String *export_path = get_export_path(&app_state->export_path, hash, ".mkv");

        Path_ensure_parent_dirs(export_path);
        if (!write_file(String_cstr(export_path), mb.data, mb.size)) {
            GLog_Error("Failed to write MKV file \"%s\" to path: \"%s\"", StringView_cstr(path),
                       String_cstr(export_path));
            String_free(export_path);
            return INVALID_GL_ID;
        }
        GLog_Warning("MKV file \"%s\" been written to file: \"%s\"", StringView_cstr(path), String_cstr(export_path));
        String_free(export_path);
    }
    else {
        String *unk_file_export_path = get_export_path(&app_state->export_path, hash, ".bin");

        Path_ensure_parent_dirs(unk_file_export_path);
        if (!write_file(String_cstr(unk_file_export_path), mb.data, mb.size)) {
            GLog_Error("Failed to write unhandled file \"%s\" to path: \"%s\"", StringView_cstr(path),
                       String_cstr(unk_file_export_path));
            String_free(unk_file_export_path);
            return INVALID_GL_ID;
        }
        GLog_Warning("Unhandled file \"%s\" been written to file: \"%s\"", StringView_cstr(path),
                     String_cstr(unk_file_export_path));
        String_free(unk_file_export_path);
    }
    mb.close(&mb);
    return output_node_id;
}
