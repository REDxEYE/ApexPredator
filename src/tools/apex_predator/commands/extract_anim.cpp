// Created by RED on 16.02.2026.
#include "../commands.h"

#include "exporter/havok_export.h"
#include "platform/app_state.h"
#include "redscore/platform/logger.h"
#include "redscore/utils/common.h"
#include "utils/hash_helper.h"
#include "apex/asset_db.h"

#include <json.hpp>

using namespace nlohmann;

void export_anim(ApexAppState &app_state, uint32 skeleton_hash, uint32 anim_hash, bool apply_root_motion);

void ExtractAnimationCommand::handle() {
    convert_to_wsl(m_game_root);
    convert_to_wsl(m_export_path);
    AssetDB db(m_db_path);
    AssetDB::set_instance(&db);
    ApexAppState app_state(m_game_root);
    app_state.skip_textures = true;
    app_state.export_path(m_export_path);
    uint32 skeleton_hash{0};

    if (is_hex(m_skeleton_path.c_str())) {
        skeleton_hash = parse_hex_u32(m_skeleton_path.c_str());
    }
    else if (is_digits(m_skeleton_path.c_str())) {
        skeleton_hash = parse_digits_u32(m_skeleton_path.c_str());
    }
    else {
        skeleton_hash = hash_string(std::filesystem::path(m_skeleton_path));
    }

    for (const auto &asset: m_animations) {
        uint32 file_hash{0};
        if (is_hex(asset.c_str())) {
            file_hash = parse_hex_u32(asset.c_str());
        }
        else if (is_digits(asset.c_str())) {
            file_hash = parse_digits_u32(asset.c_str());
        }
        else {
            file_hash = hash_string(std::filesystem::path(asset));
        }
        app_state.helper().reset();
        export_anim(app_state, skeleton_hash, file_hash, m_apply_root_motion);
    }

    AssetDB::set_instance(nullptr);
}

json extract_root_motion_info(HavokTypes::hkaAnimatedReferenceFrame *extracted_motion_base) {
    auto *extracted_motion = Havok::as<HavokTypes::hkaDefaultAnimatedReferenceFrame>(extracted_motion_base);

    auto root = json::object();
    root["frame_type"] = extracted_motion->frameType.value;
    root["duration"] = extracted_motion->duration;
    const auto &forward = extracted_motion->forward.value;
    const auto &up = extracted_motion->up.value;
    root["forward"] = {forward.x, forward.y, forward.z};
    root["up"] = {up.x, up.y, up.z};
    const auto &ref_frames = extracted_motion->referenceFrameSamples;
    auto &reference_frame_array = root["reference_frames"] = json::array();
    for (const auto &ref_frame: ref_frames) {
        const auto &pos = ref_frame.value;
        reference_frame_array.push_back({pos.x, pos.y, pos.z});
    }
    return root;
}

void export_anim(ApexAppState &app_state, uint32 skeleton_hash, uint32 anim_hash, bool apply_root_motion) {
    auto skeleton_file = app_state.manager().get_file(skeleton_hash);
    if (!skeleton_file) {
        GLog_Error("Skeleton file not found: %08X", skeleton_hash);
        return;
    }
    auto anim_file = app_state.manager().get_file(anim_hash);
    if (!anim_file) {
        GLog_Error("Animation file not found: %08X", anim_hash);
        return;
    }

    auto skeleton_tag_file = Havok::Tag::TagFile(std::move(skeleton_file));

    auto skeleton_item = Havok::Tag::get_item(skeleton_tag_file, 1);
    const auto skeleton_container = Havok::convert<HavokTypes::hkRootLevelContainer>(std::move(skeleton_item));
    const auto &variant = skeleton_container->namedVariants.front().variant;
    const auto animation_container = Havok::as<HavokTypes::hkaAnimationContainer>(variant);
    const auto &skeleton = animation_container->skeletons.front();

    auto anim_tag_file = Havok::Tag::TagFile(std::move(anim_file));
    auto anim_item = Havok::Tag::get_item(anim_tag_file, 1);
    const auto anim_container = Havok::convert<HavokTypes::hkRootLevelContainer>(std::move(anim_item));
    const auto &anim_variant = anim_container->namedVariants.front().variant;
    const auto anim_animation_container = Havok::as<HavokTypes::hkaAnimationContainer>(anim_variant);
    const auto &binding = anim_animation_container->bindings.front();

    auto anim_name = find_name(anim_hash).value_or(std::format("anim_{:08X}", anim_hash));

    export_animation(app_state, binding.get(), skeleton.get(), path_utils::stem(anim_name), apply_root_motion);

    const auto extracted_motion = extract_root_motion_info(binding->animation->extractedMotion.get());

    std::filesystem::path save_path = app_state.export_path() / anim_name;
    save_path.replace_extension("json");
    std::filesystem::create_directories(save_path.parent_path());
    std::ofstream out(save_path);
    auto json_dump = extracted_motion.dump(1);
    out.write(json_dump.c_str(), json_dump.size());
    out.close();

    save_path.replace_extension("gltf");
    const auto &helper = app_state.helper();
    if (!helper.model().scenes.empty() && !helper.model().nodes.empty()) {
        tinygltf::TinyGLTF gltf_exporter;
        if (gltf_exporter.WriteGltfSceneToFile(&helper.model(), save_path.string(), false, true, true, false)) {
            GLog_Info("Written GLTF file: {}", save_path.string());
        }
        else {
            GLog_Error("Failed to write GLTF file: {}", save_path.string());
        }
    }
}


// void extract_anims_handler(ApexAppState *app_state, const CliResult *cli_res) {
//     const char *skeleton_path_cstr = NULL;
//
//     cli_get_cstring(cli_res, "skeleton-path", &skeleton_path_cstr);
//     uint32 skeleton_path_hash = 0;
//     if (is_hex(skeleton_path_cstr)) {
//         skeleton_path_hash = parse_hex_u32(skeleton_path_cstr);
//     }
//     else if (is_digits(skeleton_path_cstr)) {
//         skeleton_path_hash = parse_digits_u32(skeleton_path_cstr);
//     }
//     else {
//         String skeleton_path_tmp = {};
//         String_from_cstr(&skeleton_path_tmp, skeleton_path_cstr);
//         Path_normalize_posix(&skeleton_path_tmp);
//         skeleton_path_hash = hash_string(&skeleton_path_tmp);
//         String_free(&skeleton_path_tmp);
//     }
//
//     IO::MemoryFile mb = {};
//     if (!ArchiveManager_get_file_by_hash(&app_state->m_archive_manager, skeleton_path_hash, &mb)) {
//         GLog_Error("Skeleton file not found: %s", skeleton_path_cstr);
//         return;
//     }
//
//     TagFile skeleton_tag_file = {};
//
//     if (memcmp(mb.data + 4, "TAG0", 4) == 0) {
//         TagFile_from_buffer(&skeleton_tag_file, (Buffer *) &mb);
//         Buffer_close((Buffer *) &mb);
//     }
//     else {
//         GLog_Error("Skeleton file is not a valid Havok TAG0 file.");
//         Buffer_close((Buffer *) &mb);
//         return;
//     }
//
//     TypedPtr *skeleton_item = TagFile_get_item(&skeleton_tag_file, 0);
//     if (skeleton_item->type_info_->hash != hkRootLevelContainer_HASH) {
//         GLog_Error("Skeleton file does not contain a hkRootLevelContainer: %s", skeleton_path_cstr);
//     INVALID_SKELETON_CLEANUP:
//         TagFile_free_item(skeleton_item);
//         TagFile_free(&skeleton_tag_file);
//         return;
//     }
//
//     const hkRootLevelContainer *root_container = (hkRootLevelContainer *) skeleton_item;
//     hkReferencedObject *variant = root_container->namedVariants.m_data->variant.ptr;
//     if (variant->type_info_->hash != hkaAnimationContainer_HASH) {
//         GLog_Error("Skeleton root_container file does not contain a hkaAnimationContainer: %s", skeleton_path_cstr);
//         goto INVALID_SKELETON_CLEANUP;
//     }
//     const hkaAnimationContainer *animation_container = (const hkaAnimationContainer *) variant;
//     const hkaSkeleton *skeleton = NULL;
//     if (animation_container->skeletons.m_size > 0) {
//         skeleton = animation_container->skeletons.m_data[0].ptr;
//     }
//
//     const char **file_paths = NULL;
//     size_t file_path_count = 0;
//     cli_get_array_string(cli_res, "animations", &file_paths, &file_path_count);
//
//     for (int file_id = 0; file_id < file_path_count; ++file_id) {
//         const char *animation_path_cstr = file_paths[file_id];
//         uint32 animation_path_hash = 0;
//         if (is_hex(animation_path_cstr)) {
//             animation_path_hash = parse_hex_u32(animation_path_cstr);
//         }
//         else if (is_digits(animation_path_cstr)) {
//             animation_path_hash = parse_digits_u32(animation_path_cstr);
//         }
//         else {
//             String animation_path_tmp = {};
//             String_from_cstr(&animation_path_tmp, animation_path_cstr);
//             Path_normalize_posix(&animation_path_tmp);
//             animation_path_hash = hash_string(&animation_path_tmp);
//         }
//
//         if (!ArchiveManager_get_file_by_hash(&app_state->m_archive_manager, animation_path_hash, &mb)) {
//             GLog_Error("Animation file not found: %s", animation_path_cstr);
//             continue;
//         }
//         TagFile anim_tag_file = {};
//
//         if (memcmp(mb.data + 4, "TAG0", 4) == 0) {
//             TagFile_from_buffer(&anim_tag_file, (Buffer *) &mb);
//             Buffer_close((Buffer *) &mb);
//         }
//         else {
//             GLog_Error("Skeleton file is not a valid Havok TAG0 file.");
//             Buffer_close((Buffer *) &mb);
//             TagFile_free(&skeleton_tag_file);
//             continue;
//         }
//
//         // export_animation
//         TypedPtr *anim_item = TagFile_get_item(&anim_tag_file, 0);
//         if (anim_item->type_info_->hash != hkRootLevelContainer_HASH) {
//             GLog_Error("Animation file does not contain a hkRootLevelContainer: %s", animation_path_cstr);
//         INVALID_ANIM_CLEANUP:
//             TagFile_free_item(anim_item);
//             TagFile_free(&anim_tag_file);
//             continue;
//         }
//         const hkRootLevelContainer *anim_root_container = (hkRootLevelContainer *) anim_item;
//         hkReferencedObject *anim_variant = anim_root_container->namedVariants.m_data->variant.ptr;
//         if (anim_variant->type_info_->hash != hkaAnimationContainer_HASH) {
//             GLog_Error("Animation root_container file does not contain a hkaAnimationContainer: %s",
//                        animation_path_cstr);
//             goto INVALID_ANIM_CLEANUP;
//         }
//         const hkaAnimationContainer *anim_animation_container = (const hkaAnimationContainer *) anim_variant;
//         assert(anim_animation_container->bindings.m_size==1 &&
//             "Only single animation binding per container is supported currently.");
//
//         const hkaAnimationBinding *binding = anim_animation_container->bindings.m_data[0].ptr;
//         String anim_file_name = {};
//         StringView animation_path_tmp = find_name(animation_path_hash);
//         if (sv_is_null(animation_path_tmp)) {
//             String_format(&anim_file_name, "anim_%08X", animation_path_hash);
//         }
//         else {
//             Path_filename_sv(animation_path_tmp, &anim_file_name);
//         }
//
//         GLTFContext_init(&app_state->m_gltf_context, String_cstr(&anim_file_name));
//         String_free(&anim_file_name);
//
//         export_animation(app_state, binding, skeleton, animation_path_tmp);
//
//         String export_file_path = {};
//         String_copy_from(&export_file_path, &app_state->export_path);
//         Path_join_sv(&export_file_path, animation_path_tmp);
//         Path_ensure_parent_dirs(&export_file_path);
//         Path_replace_extension_inplace(&export_file_path, "gltf");
//         GLTFContext_set_save_path(&app_state->m_gltf_context, &export_file_path);
//         String_free(&export_file_path);
//
//         GLTFContext_write_and_free(&app_state->m_gltf_context);
//
//         TagFile_free_item(anim_item);
//         TagFile_free(&anim_tag_file);
//     }
//     TagFile_free_item(skeleton_item);
//     TagFile_free(&skeleton_tag_file);
// // }
