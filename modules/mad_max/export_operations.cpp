#include "exporter/common_export.h"
#include "exporter/havok_export.h"
#include "platform/app_state.h"
#include "apex/adf/adf.h"
#include "apex/adf/generated/adf_types.h"
#include "apex/asset_db.h"
#include "redscore/platform/logger.h"
#include "tracy/Tracy.hpp"
#include <cstring>
#include <fstream>
using nlohmann::json;

void normal_export(ApexAppState &app_state, const uint64 asset_hash) {
    ZoneScoped
    auto buffer = app_state.manager().get(asset_hash);
    if (!buffer) throw std::runtime_error("Asset not found");
    const auto &bytes = buffer->cbuffer();
    const bool is_packfile = bytes.size() >= sizeof(HAVOK2014_MAGIC)
            && std::memcmp(bytes.data(), HAVOK2014_MAGIC, sizeof(HAVOK2014_MAGIC)) == 0;
    if (!is_packfile && bytes.size() < sizeof(ADF::SmallHeader))
        throw std::runtime_error("Asset is not an ADF or Havok packfile; use --raw for other formats");

    json instances;
    if (is_packfile) {
        auto &builder = app_state.models();
        builder.reset();
        const auto name = find_asset_name(asset_hash);
        const auto asset_path = name && !name->empty() ? std::string(*name)
                                                       : std::format("{:08X}.bin", asset_hash);
        const auto node = export_file(app_state, asset_hash, std::move(buffer), asset_path);
        if (node) builder.add_to_scene(node);
        if (builder.scene.roots.empty()) throw std::runtime_error("Havok skeleton export produced no scene");
        auto save_path = app_state.export_path() / asset_path;
        save_path.replace_extension("gltf");
        std::filesystem::create_directories(save_path.parent_path());
        if (!VM::save_gltf(builder.scene, save_path))
            throw std::runtime_error("Failed to write GLTF file: " + save_path.string());
        GLog_Info("Written GLTF file: {}", save_path.string());
        return;
    } else {
        uint32 alignment = 0;
        if (std::memcmp(bytes.data(), ADF_SMALL_MAGIC, 4) == 0) {
            uint32 type_hash;
            std::memcpy(&type_hash, bytes.data() + 4, sizeof(type_hash));
            const auto type = adf_type_info.find(type_hash);
            if (type == adf_type_info.end() || !type->second->new_instance)
                throw std::runtime_error(std::format("No Mad Max ADF binding for type 0x{:08X}", type_hash));
            alignment = type->second->alignment;
        } else if (std::memcmp(bytes.data(), ADF_MAGIC, 4) != 0) {
            throw std::runtime_error("Asset is not an ADF or Havok packfile; use --raw for other formats");
        }

        auto adf = ADF::ADFFile::from_buffer(std::move(buffer), alignment);
        instances = json::array();
        for (uint32 i = 0; i < adf.instances().size(); ++i) {
            const auto &info = adf.instances()[i];
            auto instance = adf.read_instance(i);
            if (!instance)
                throw std::runtime_error(std::format("No Mad Max ADF binding for type 0x{:08X}", info.type_hash));
            instances.push_back({{"type_hash", std::format("0x{:08X}", info.type_hash)},
                                 {"value", instance->to_json()}});
        }
    }

    const auto name = find_asset_name(asset_hash);
    const auto asset_path = name && !name->empty() ? std::string(*name)
                                                   : std::format("{:08X}.bin", asset_hash);
    auto save_path = app_state.export_path() / asset_path;
    save_path.replace_extension("json");
    std::filesystem::create_directories(save_path.parent_path());
    std::ofstream out(save_path);
    out << instances.dump(2);
    if (!out) throw std::runtime_error("Failed to write JSON: " + save_path.string());
    GLog_Info("File \"{}\" extracted to \"{}\"", asset_hash, save_path.string());
}
/*
json extract_root_motion_info(HavokTypes::hkaAnimatedReferenceFrame *extracted_motion_base) {
    auto *extracted_motion = Havok::as<HavokTypes::hkaDefaultAnimatedReferenceFrame>(extracted_motion_base);
    if (extracted_motion == nullptr) return json::object();

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
    if (!skeleton_file) throw std::runtime_error("Skeleton file not found");
    auto anim_file = app_state.manager().get(anim_hash);
    if (!anim_file) throw std::runtime_error("Animation file not found");

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
        } else {
            throw std::runtime_error("Failed to write GLTF file: " + save_path.string());
        }
    }
}
*/