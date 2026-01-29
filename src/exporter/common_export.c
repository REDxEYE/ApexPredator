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
#include "../../include/havok/generated/havok_generated.h"
#include "utils/path.h"
#include "platform/logger.h"
#include "platform/memory_profiling.h"


GL_ID export_file(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                  Havok_TypeLibrary *havok_lib,
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
        GLog_Error("File not found");
        return INVALID_GL_ID;
    }
    GL_ID output_node_id = INVALID_GL_ID;

    if (memcmp(mb.data, ADF_MAGIC, 4) == 0) {
        output_node_id = export_adf_file_from_buffer(context, archive_manager, lib, havok_lib, hash, path, &mb,
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

        SArchive *sarc = SArchive_new((Buffer *) section_buffer); // sarc is now owner of buffer
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
        output_node_id = export_epe(context, archive_manager, lib, havok_lib, root_node, hash, path, export_path);
        RuntimeNode_free(root_node);
    }
    else if (memcmp(mb.data + 4, "TAG0", 4) == 0) {
        TagFile tag_file = {0};
        TagFile_from_buffer(&tag_file, (Buffer *) &mb);
        Havok_TypeLibrary_copy_from_tag_file(havok_lib, &tag_file);
        // for (int item_id = 1; item_id < tag_file.items.count; ++item_id) {
        //     const HKItem *item = &tag_file.items.items[item_id];
        //     const uint32 type_hash = hash_string(&tag_file.types.items[item->type].stable_name);
        //     const HavokType *item_type = DM_get(&havok_lib->types, type_hash);
        //     if (item_type == NULL) {
        //         GLog_Error("No type data for item %d of type hash 0x%08X", item_id, type_hash);
        //         continue;
        //     }
        //     GLog_Info("TagFile Item %d: Type: %s size: %u",
        //               item_id,
        //               String_cstr(&item_type->name),
        //               item->count*item_type->size
        //     );
        // }
        const HKItem *item = &tag_file.items.items[1];
        const uint32 type_hash = hash_string(&tag_file.types.items[item->type].stable_name);
        const HavokType *item_type = DM_get(&havok_lib->types, type_hash);
        const HavokTypeInfo *type_info = *(HavokTypeInfo **) DM_get(&HAVOK_TYPES_type_info, type_hash);
        void *item_obj = mp_malloc(type_info->size);
        type_info->init(item_obj);
        type_info->read(item_obj, &tag_file, &tag_file.data.items[item->offset]);

        if (String_cequals(&item_type->name, "hkRootLevelContainer")) {
            hkRootLevelContainer *root_level_container = (hkRootLevelContainer *) item_obj;
            for (int i = 0; i < root_level_container->namedVariants.m_size; ++i) {
                hkRootLevelContainer__NamedVariant *variant = &root_level_container->namedVariants.m_data[i];
                if (strcmp(variant->className.m_data, "hkaAnimationContainer") == 0) {
                    hkaAnimationContainer *animation_container = (hkaAnimationContainer *) variant->variant.ptr;
                    for (int skeleton_id = 0; skeleton_id < animation_container->skeletons.m_size; ++skeleton_id) {
                        const hkaSkeleton *skeleton = animation_container->skeletons.m_data[skeleton_id].ptr;
                        output_node_id = export_skeleton(context, skeleton, havok_lib);
                    }
                    for (int animation_id = 0; animation_id < animation_container->animations.m_size; ++animation_id) {
                        const hkaAnimation *animation = animation_container->animations.m_data[animation_id].ptr;
                        if (animation->type_info_->hash == hkaAnimation_HASH) {
                            GLog_Warning("Raw hkaAnimation cannot be exported");
                        }else if (animation->type_info_->hash == hkaSplineCompressedAnimation_HASH) {
                            hkaSplineCompressedAnimation *spline_animation = (hkaSplineCompressedAnimation *) animation;
                        }
                    }
                }
            }
            String bsk_export_path = {};
            Path_join(&bsk_export_path, export_path);
            Path_join(&bsk_export_path, path);
            Path_ensure_parent_dirs(&bsk_export_path);
            String_append_cstr(&bsk_export_path, ".gltf");
            GLTFContext_set_save_path(context, &bsk_export_path);
            String_free(&bsk_export_path);
        }
        else {
            GLog_Warning("Unhandled havok type: %s", String_cstr(&item_type->name));
        }

        JsonContext tmp;
        String unk_file_export_path = {0};
        Path_join(&unk_file_export_path, export_path);
        Path_join(&unk_file_export_path, path);
        Path_ensure_parent_dirs(&unk_file_export_path);
        String json_output = {0};
        Path_replace_extension(&unk_file_export_path, "json", &json_output);
        FILE *f = fopen(String_cstr(&json_output), "wb");
        jsonInit(&tmp, f);
        jsonBeginObject(&tmp);
        ((TypedPtr*)item_obj)->type_info_->print(item_obj, &tmp);
        jsonEndObject(&tmp);
        fclose(f);
        String_free(&unk_file_export_path);
        String_free(&json_output);

        type_info->free(item_obj);
        mp_free(item_obj);
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
