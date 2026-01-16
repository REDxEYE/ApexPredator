// Created by RED on 12.01.2026.

#include "../../include/exporter/common_export.h"

#include "../../include/exporter/adf_export.h"
#include "apex/avtx.h"
#include "apex/rtpc.h"
#include "apex/sarc.h"
#include "apex/aaf/aaf.h"
#include "apex/adf/adf.h"
#include "exporter/ddsc_export.h"
#include "exporter/epe_export.h"
#include "exporter/havok_export.h"
#include "havok/havok_generated.h"
#include "utils/path.h"


GL_ID export_file(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib, Havok_TypeLibrary *havok_lib,
                  const String *path,
                  uint32 hash, const String *export_path) {
    assert(context!=NULL && "context must be initialized");

    MemoryBuffer mb = {0};
    if (!ArchiveManager_get_file_by_hash(archive_manager, hash, &mb)) {
        printf("File not found\n");
        return INVALID_GL_ID;
    }
    GL_ID output_node_id = INVALID_GL_ID;

    if (memcmp(mb.data, ADF_MAGIC, 4) == 0) {
        output_node_id = export_adf_file_from_buffer(context, archive_manager, lib, havok_lib, hash, path, &mb, export_path);
    } else if (memcmp(mb.data, AAF_MAGIC, 4) == 0) {
        AAFArchive aaf_archive = {0};
        AAFArchive_from_buffer(&aaf_archive, (Buffer *) &mb);
        MemoryBuffer *section_buffer = MemoryBuffer_new();
        if (!AAFArchive_get_data(&aaf_archive, section_buffer)) {
            printf("[ERROR]: Failed to get AAF section 0\n");
            return INVALID_GL_ID;
        }

        SArchive *sarc = SArchive_new((Buffer *) section_buffer); // sarc is now owner of buffer
        ArchiveManager_add(archive_manager, (Archive *) sarc);
        // Archive_print_files((Archive *) sarc);
        AAFArchive_free(&aaf_archive);
    } else if (memcmp(mb.data, AVTX_MAGIC, 4) == 0) {
        export_ddsc(archive_manager, hash, &mb, path, export_path);
    } else if (memcmp(mb.data, RTPC_MAGIC, 4) == 0) {
        RuntimeNode *root_node = RuntimeContainer_from_buffer((Buffer *) &mb);
        if (root_node==NULL) {
            return INVALID_GL_ID;
        }

        // RuntimeNode_print(root_node, stdout, 0);
        // String epe_json = {0};
        // String_init(&epe_json, 8192);
        // RuntimeNode_emit_json(root_node, &epe_json, 0);
        // printf("%s\n", String_data(&epe_json));
        output_node_id = export_epe(context, archive_manager, lib, havok_lib, root_node, hash, path, export_path);
        RuntimeNode_free(root_node);
    } else if (memcmp(mb.data + 4, "TAG0", 4) == 0) {
        TagFile tag_file = {0};
        TagFile_from_buffer(&tag_file, (Buffer *) &mb);
        HavokTypeLib_copy_from_tag_file(havok_lib, &tag_file);
        const HKItem *item = &tag_file.items.items[1];
        const uint32 type_hash = hash_string(&tag_file.types.items[item->type].name);
        const HavokType *item_type = DM_get(&havok_lib->types, type_hash);
        const HAVOK_ObjectMethods *type_methods = DM_get(&havok_lib->object_functions, type_hash);
        void *item_obj = (void *) malloc(item_type->size * item->count);
        type_methods->read(&tag_file, havok_lib, item_obj, &tag_file.data.items[item->offset]);
        // JsonContext ctx;
        // jsonInit(&ctx, stdout);
        // type_methods->print(item_obj, havok_lib, &ctx);
        if (strcmp(String_data(&item_type->name), "hkRootLevelContainer") == 0) {
            hkRootLevelContainer *root_level_container = (hkRootLevelContainer *) item_obj;
            for (int i = 0; i < root_level_container->namedVariants.m_size; ++i) {
                hkRootLevelContainer__NamedVariant *variant = &root_level_container->namedVariants.m_data[i];
                if (strcmp(variant->className.stringAndFlag, "hkaAnimationContainer") == 0) {
                    hkaAnimationContainer *animation_container = (hkaAnimationContainer *) variant->variant.ptr;
                    for (int skeleton_id = 0; skeleton_id < animation_container->skeletons.m_size; ++skeleton_id) {
                        hkaSkeleton *skeleton = animation_container->skeletons.m_data[skeleton_id].ptr;
                        output_node_id = export_skeleton(context, skeleton, havok_lib);
                    }
                }
            }
            String bsk_export_path = {};
            Path_join(&bsk_export_path, export_path);
            Path_join(&bsk_export_path, path);
            Path_ensure_parent_dirs(&bsk_export_path);
            String_append_cstr(&bsk_export_path, ".gltf");
            GLTFContext_set_save_path(context, &bsk_export_path);
        } else {
            printf("Unhandled havok type: %s\n", String_data(&item_type->name));
        }
        // type_methods->free(item_obj, havok_lib);
        TagFile_free(&tag_file);
    } else {
        String unk_file_export_path = {};
        Path_join(&unk_file_export_path, export_path);
        Path_join(&unk_file_export_path, path);
        Path_ensure_parent_dirs(&unk_file_export_path);
        FILE *f = fopen(String_data(&unk_file_export_path), "wb");
        fwrite(mb.data, 1, mb.size, f);
        fclose(f);
        printf("Unhandled file \"%s\" been written to file: \"%s\"", String_data(path),
               String_data(&unk_file_export_path));
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
