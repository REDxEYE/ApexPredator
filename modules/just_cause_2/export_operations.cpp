#include "platform/app_state.h"
#include "exporter/common_export.h"
// #include "exporter/havok_export.h"
#include "exporter/rtpc_export.h"
#include "apex/asset_db.h"
#include "redscore/platform/logger.h"
#include "redscore/utils/simple_fileio.h"
#include "redscore/utils/common.h"
#include "tracy/Tracy.hpp"
#include <fstream>
using nlohmann::json;


void raw_export(ApexAppState &app_state, const uint64 asset_hash) {
    ZoneScoped
    const auto asset_path = find_asset_name(asset_hash)
            .or_else([&] { return std::optional{std::format("{:08X}.bin", asset_hash)}; })
            .value();


    const std::filesystem::path save_path = app_state.export_path() / asset_path;
    std::filesystem::create_directories(save_path.parent_path());

    const auto mb = app_state.manager().get(asset_hash);
    if (!mb) {
        GLog_Error("File not found: {}", asset_hash);
        throw std::runtime_error("Asset not found");
    }

    write_file(save_path, mb->cbuffer());
    GLog_Info("File \"{}\" extracted to \"{}\"", asset_hash, save_path.string());
}

void normal_export(ApexAppState &app_state, const uint64 asset_hash) {
    ZoneScoped
    const auto asset_path = find_asset_name(asset_hash)
            .or_else([&] { return std::optional{std::format("{:08X}.bin", asset_hash)}; })
            .value();

    std::filesystem::path save_path = app_state.export_path() / asset_path;
    save_path.replace_extension("gltf");

    auto &builder = app_state.models();
    builder.reset();
    const auto node = export_file(app_state, asset_hash);
    if (node) {
        builder.add_to_scene(node);
    }
    if (!builder.scene.roots.empty()) {
        std::filesystem::create_directories(save_path.parent_path());
        if (VM::save_gltf(builder.scene, save_path)) {
            GLog_Info("Written GLTF file: {}", save_path.string());
        } else {
            throw std::runtime_error("Failed to write GLTF file: " + save_path.string());
        }

    }
}
/*
json extract_root_motion_info(HavokTypes::hkaAnimatedReferenceFrame *extracted_motion_base) {
    auto *extracted_motion = Havok::as<HavokTypes::hkaDefaultAnimatedReferenceFrame>(extracted_motion_base);
    if (extracted_motion==nullptr) {
        return json::object();
    }
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

void export_anim(ApexAppState &app_state, uint64 skeleton_hash, uint64 anim_hash, bool apply_root_motion) {
    auto skeleton_file = app_state.manager().get(skeleton_hash);
    if (!skeleton_file) {
        throw std::runtime_error("Skeleton file not found");
    }
    auto anim_file = app_state.manager().get(anim_hash);
    if (!anim_file) {
        throw std::runtime_error("Animation file not found");
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

    auto anim_name = find_asset_name(anim_hash).value_or(std::format("anim_{:08X}", anim_hash));

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
    const auto &helper = app_state.models();
    if (!helper.scene.roots.empty()) {
        if (VM::save_gltf(helper.scene, save_path)) {
            GLog_Info("Written GLTF file: {}", save_path.string());
        }
        else {
            throw std::runtime_error("Failed to write GLTF file: " + save_path.string());
        }
    }
}
*/

