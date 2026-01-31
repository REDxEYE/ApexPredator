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
#include "exporter/epe_export.h"
#include "exporter/havok_export.h"
#include "utils/path.h"
#include "platform/logger.h"
#include "utils/memory_profiling.h"

void mount_archive(const ArchiveManager *manager, const uint32 hash) {
    if (ArchiveManager_mounted(manager, hash)) {
        return;
    }
    MemoryBuffer mb = {0};
    if (!ArchiveManager_get_file_by_hash(manager, hash, &mb)) {
        String *file_name = find_name32(hash);
        if (file_name != NULL) {
            GLog_Error("File \"%s\" not found", String_cstr(file_name));
            String_free(file_name);
        }
        else {
            GLog_Error("File 0x%08X not found", hash);
        }
        return;
    }
    if (memcmp(mb.data, AAF_MAGIC, 4) == 0) {
        String *file_name = find_name32(hash);
        if (file_name != NULL) {
            GLog_Info("Mounting AAF archive \"%s\"", String_cstr(file_name));
            String_free(file_name);
        }else {
            GLog_Info("Mounting AAF archive with hash 0x%08X", hash);
        }
        AAFArchive aaf_archive = {0};
        AAFArchive_from_buffer(&aaf_archive, (Buffer *) &mb);
        MemoryBuffer *section_buffer = MemoryBuffer_new();
        if (!AAFArchive_get_data(&aaf_archive, section_buffer)) {
            GLog_Error("Failed to get AAF section 0");
            return;
        }

        SArchive *sarc = SArchive_new((Buffer *) section_buffer, hash); // sarc is now owner of buffer
        ArchiveManager_add(manager, (Archive *) sarc);
        AAFArchive_free(&aaf_archive);
    }
}


GL_ID export_file(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                  const String *path,
                  uint32 hash, const String *export_path) {
    if (context == NULL) {
        GLog_Error("GLTF context is not initialized!");
        assert(context!=NULL && "context must be initialized");
        exit(1);
    }

    GLog_Info("Exporting file: %s", path!=NULL ? String_cstr(path) : "unknown");
    MemoryBuffer mb = {0};
    if (!ArchiveManager_get_file_by_hash(archive_manager, hash, &mb)) {
        if (path != NULL) {
            GLog_Error("File \"%s\" not found", String_cstr(path));
            return INVALID_GL_ID;
        }
        String *file_name = find_name32(hash);
        if (file_name != NULL) {
            GLog_Error("File \"%s\" not found", String_cstr(file_name));
            String_free(file_name);
            return INVALID_GL_ID;
        }
        GLog_Error("File 0x%08X not found", hash);
        return INVALID_GL_ID;
    }
    GL_ID output_node_id = INVALID_GL_ID;

    if (memcmp(mb.data, ADF_MAGIC, 4) == 0) {
        output_node_id = export_adf_file_from_buffer(context, archive_manager, lib, hash, path, &mb,
                                                     export_path);
    }
    else if (memcmp(mb.data, AAF_MAGIC, 4) == 0) {
        AAFArchive aaf_archive = {0};
        AAFArchive_from_buffer(&aaf_archive, (Buffer *) &mb);
        MemoryBuffer *section_buffer = MemoryBuffer_new();
        if (!AAFArchive_get_data(&aaf_archive, section_buffer)) {
            GLog_Error("Failed to get AAF section 0");
            return INVALID_GL_ID;
        }

        SArchive *sarc = SArchive_new((Buffer *) section_buffer, hash); // sarc is now owner of buffer
        ArchiveManager_add(archive_manager, (Archive *) sarc);
        // Archive_print_files((Archive *) sarc);
        AAFArchive_free(&aaf_archive);
    }
    else if (memcmp(mb.data, AVTX_MAGIC, 4) == 0) {
        export_ddsc(archive_manager, hash, &mb, path, export_path);
    }
    else if (memcmp(mb.data, RTPC_MAGIC, 4) == 0) {
        RuntimeNode *root_node = RuntimeContainer_from_buffer((Buffer *) &mb);
        if (root_node == NULL) {
            return INVALID_GL_ID;
        }

        // RuntimeNode_print(root_node, stdout, 0);
        // String epe_json = {0};
        // String_init(&epe_json, 8192);
        // RuntimeNode_emit_json(root_node, &epe_json, 0);
        // printf("%s\n", String_data(&epe_json));
        output_node_id = export_epe(context, archive_manager, lib, root_node, hash, path, export_path);
        RuntimeNode_free(root_node);
    }
    else if (memcmp(mb.data + 4, "TAG0", 4) == 0) {
        TagFile tag_file = {0};
        TagFile_from_buffer(&tag_file, (Buffer *) &mb);
        output_node_id = export_havok_file(context, &tag_file, path, export_path);
        TagFile_free(&tag_file);
    }
    else {
        String unk_file_export_path = {};
        Path_join(&unk_file_export_path, export_path);
        Path_join(&unk_file_export_path, path);
        Path_ensure_parent_dirs(&unk_file_export_path);
        FILE *f = fopen(String_cstr(&unk_file_export_path), "wb");
        fwrite(mb.data, 1, mb.size, f);
        fclose(f);
        GLog_Warning("Unhandled file \"%s\" been written to file: \"%s\"", String_cstr(path),
                     String_cstr(&unk_file_export_path));
        String_free(&unk_file_export_path);
    }
    mb.close(&mb);
    // if (!IS_VALID_GL_ID(output_node_id)) {
    //     String path_stem = {};
    //     if (path != NULL) {
    //         Path_filename(path, &path_stem);
    //     }else {
    //         String_from_cstr(&path_stem, "file_");
    //         String_append_format(&path_stem, "%08X", hash);
    //     }
    //     output_node_id = GLTFContext_add_node(context, String_data(&path_stem));
    //     String_free(&path_stem);
    // }
    return output_node_id;
}
