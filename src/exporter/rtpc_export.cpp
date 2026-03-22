// Created by RED on 12.01.2026.

#include "exporter/rtpc_export.h"

#include "tiny_gltf.h"
#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"

#include "apex/hashes.h"
#include "exporter/havok_export.h"
#include "exporter/adf_export.h"
#include "exporter/common_export.h"
#include "redscore/platform/logger.h"
#include "tracy/Tracy.hpp"

using namespace tinygltf;

Value convert_to_value(const nlohmann::json& j) {
    if (j.is_null()) {
        return {};
    }
    if (j.is_boolean()) {
        return  Value(j.get<bool>());
    }
    if (j.is_number_float()) {
        return  Value(j.get<float>());
    }
    if (j.is_number_integer()) {
        return  Value(j.get<int>());
    }
    if (j.is_string()) {
        return  Value(j.get<std::string>());
    }
    if (j.is_array()) {
        Value::Array array;
        for (const auto &item : j) {
            array.push_back(convert_to_value(item));
        }
        return  Value(array);
    }
    if (j.is_object()) {
        Value::Object object;
        for (const auto &[key, key_value] : j.items()) {
            object.emplace(key, convert_to_value(key_value));
        }
        return  Value(object);
    }

    throw std::runtime_error("Unsupported type");
}

void add_extras(const RuntimeNode &node, const GltfHelper::Handle<Node> &output_node) {
    const nlohmann::json extra = node.to_json();

    output_node->extras = convert_to_value(extra);
}

glm::mat4 get_node_matrix(const GltfHelper::Handle<Node> &target_node) {
    auto out = glm::identity<glm::mat4>();
    if (!target_node->matrix.empty()) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                out[j][i] = static_cast<float32>(target_node->matrix.at(i * 4 + j));
            }
        }
    }
    else {
        if (!target_node->translation.empty()) {
            const glm::vec3 tmp = {
                target_node->translation.at(0),
                target_node->translation.at(1),
                target_node->translation.at(2)
            };
            out = glm::translate(out, tmp);
        }
        if (!target_node->rotation.empty()) {
            const glm::quat tmp = {
                static_cast<float32>(target_node->rotation.at(3)),
                static_cast<float32>(target_node->rotation.at(0)),
                static_cast<float32>(target_node->rotation.at(1)),
                static_cast<float32>(target_node->rotation.at(2))
            };
            out *= glm::mat4_cast(tmp);
        }
        if (!target_node->scale.empty()) {
            const glm::vec3 tmp = {
                target_node->scale.at(0),
                target_node->scale.at(1),
                target_node->scale.at(2)
            };
            out = glm::scale(out, tmp);
        }
    }
    return out;
}

glm::mat4 calculate_global_node_matrix(GltfHelper &helper, const GltfHelper::Handle<Node> target) {
    if (!target.is_valid()) {
        return glm::identity<glm::mat4>();
    }
    const auto parent_node = helper.get_parent(target);

    if (parent_node.is_valid()) {
        const glm::mat4 parent_matrix = calculate_global_node_matrix(helper, parent_node);
        const glm::mat4 local_matrix = get_node_matrix(target);
        return parent_matrix * local_matrix;
    }
    return get_node_matrix(target);
}

void process_children(AppState &app_state, const RuntimeNode &node, const uint32 path_hash,
                      const GltfHelper::Handle<Node> &parent_gltf_node) {
    for (const auto &child: node.children()) {
        process_rtpc_node(app_state, child, path_hash, parent_gltf_node);
    }
}

void set_world_matrix(const GltfHelper::Handle<Node> &gltf_node, const RuntimeNode &node) {
    if (!node.has("world"))
        return;
    const auto &matrix = node.get<glm::mat4>("world");
    if (matrix != glm::identity<glm::mat4>())
        GltfHelper::set_node_matrix(*gltf_node, matrix);
}

void handle_CCharacter(AppState &app_state,
                       const RuntimeNode &node, const uint32 path_hash,
                       const GltfHelper::Handle<Node> &parent_gltf_node) {
    GltfHelper &helper = app_state.helper();

    if (!node.has(0xE8129FE6)) {
        GLog_Error("Failed to get model property for CCharacter");
        return;
    }
    if (!node.has(0x26FA86FE)) {
        GLog_Error("Failed to get skeleton property for CCharacter");
        return;
    }

    const auto &model_filename = node.get<std::string>(0xE8129FE6);
    const auto &skeleton_filename = node.get<std::string>(0x26FA86FE);

    std::filesystem::path skeleton_bsk_name = skeleton_filename;
    skeleton_bsk_name.replace_extension(".bsk");

    const auto skeleton_node = export_file(app_state, hash_string(skeleton_bsk_name));

    if (!skeleton_node.is_valid()) {
        GLog_Error("Failed to export skeleton for CCharacter: {}", skeleton_bsk_name.string());
        throw std::runtime_error("Failed to export skeleton for CCharacter");
    }
    const auto skin = helper.current_skin();
    if (!skin.is_valid()) {
        GLog_Error("Failed to get current skin for CCharacter");
        throw std::runtime_error("Failed to get current skin for CCharacter");
    }

    const auto root_bone = helper.get<Node>(skin->joints[0]);
    if (!root_bone.is_valid()) {
        GLog_Error("Failed to get root bone for CCharacter");
        throw std::runtime_error("Failed to get root bone for CCharacter");
    }
    helper.set_parent(root_bone, parent_gltf_node);

    const auto output_node = export_adf_file(app_state, hash_string(model_filename));

    add_extras(node, output_node);
    set_world_matrix(output_node, node);
    if (parent_gltf_node.is_valid())
        helper.set_parent(parent_gltf_node, output_node);
    else {
        GLog_Warning("Invalid parent setup: 0x{:08X}", node.name_hash());
    }

    process_children(app_state, node, path_hash, output_node);
    if (skin.is_valid()) {
        helper.pop_skin();
    }
}

void handle_CSecondaryMotionAttachment(AppState &app_state,
                                       const RuntimeNode &node, const uint32 path_hash,
                                       const GltfHelper::Handle<Node> &parent_gltf_node) {
    auto &helper = app_state.helper();
    if (!node.has("model")) {
        GLog_Error("Failed to get model property for CSecondaryMotionAttachment");
        return;
    }
    if (!node.has(0x26FA86FE)) {
        GLog_Error("Failed to get skeleton property for CSecondaryMotionAttachment");
        return;
    }
    const auto &model_filename = node.get<std::string>("model");
    const auto &skeleton_filename = node.get<std::string>(0x26FA86FE);
    std::filesystem::path skeleton_bsk_name = skeleton_filename;
    skeleton_bsk_name.replace_extension(".bsk");
    const auto skeleton_node = export_file(app_state, hash_string(skeleton_bsk_name));
    if (!skeleton_node.is_valid()) {
        GLog_Error("Failed to export skeleton for CSecondaryMotionAttachment: {}", skeleton_bsk_name.string());
        throw std::runtime_error("Failed to export skeleton for CSecondaryMotionAttachment");
    }
    const auto skin = helper.current_skin();
    if (!skin.is_valid()) {
        GLog_Error("Failed to get current skin for CSecondaryMotionAttachment");
    }

    const auto output_node = export_adf_file(app_state, hash_string(model_filename));
    set_world_matrix(output_node, node);
    add_extras(node, output_node);
    if (parent_gltf_node.is_valid())
        helper.set_parent(parent_gltf_node, output_node);
    else {
        GLog_Warning("Invalid parent setup: 0x{:08X}", node.name_hash());
    }
    process_children(app_state, node, path_hash, output_node);
    if (skin.is_valid()) {
        helper.pop_skin();
    }
}

void handle_CDamageableCharacterPart(AppState &app_state,
                                     const RuntimeNode &node, const uint32 path_hash,
                                     const GltfHelper::Handle<Node> &parent_gltf_node) {
    auto &helper = app_state.helper();
    std::string node_name = {};
    if (node.has("name"))
        node_name = node.get<std::string>("name");
    else
        node_name = find_name(node.name_hash()).value_or(std::format("node_{:08X}", node.name_hash()));

    const auto output_node = helper.make<Node>();
    output_node->name = node_name;

    set_world_matrix(output_node, node);
    add_extras(node, output_node);

    const auto current_skin = helper.current_skin();
    if (current_skin.is_valid() && node.has(0x4d67eec5)) {
        const auto &parent_bone_name = node.get<std::string>(0x4d67eec5);
        const auto parent_bone = helper.find_node_in_skin(current_skin, parent_bone_name);
        if (parent_bone.is_valid()) {
            glm::mat4 node_global_matrix = calculate_global_node_matrix(helper, parent_bone);
            node_global_matrix = glm::inverse(node_global_matrix);
            GltfHelper::set_node_matrix(*output_node, node_global_matrix);
            helper.set_parent(parent_bone, output_node);
        }
        else {
            GLog_Warning("Parent bone not found: {}", parent_bone_name);
        }
    }
    else if (parent_gltf_node.is_valid()) {
        helper.set_parent(parent_gltf_node, output_node);
    }
    else {
        GLog_Warning("Invalid parent setup: {}", node.name_hash());
    }

    process_children(app_state, node, path_hash, output_node);
}

void handle_CRigidObject(AppState &app_state, const RuntimeNode &node, const uint32 path_hash,
                         const GltfHelper::Handle<Node> &parent_gltf_node) {
    auto &helper = app_state.helper();

    const auto model_filename_hash = node.get<uint32>("filename");
    if (model_filename_hash == 0) {
        GLog_Error("Failed to get model property for CRigidObject");
        return;
    }
    auto output_node = export_adf_file(app_state, model_filename_hash);
    if (output_node.is_valid()) {
        set_world_matrix(output_node, node);

        if (parent_gltf_node.is_valid())
            helper.set_parent(parent_gltf_node, output_node);
        else {
            GLog_Warning("Invalid parent setup: 0x{:08X}", node.name_hash());
        }
    }
    else {
        const auto model_filename = find_name(model_filename_hash).or_else([&] {
            return find_name(node.name_hash());
        }).value_or(std::format("model_{:08X}", model_filename_hash));

        output_node = helper.make<Node>();
        output_node->name = model_filename;
        set_world_matrix(output_node, node);
        add_extras(node, output_node);
    }
    process_children(app_state, node, path_hash, output_node);
}

void handle_CSkeletalAnimatedObject(AppState &app_state, const RuntimeNode &node, const uint32 path_hash,
                                    const GltfHelper::Handle<Node> &parent_gltf_node) {
    auto &helper = app_state.helper();

    if (!node.has(0x0f94740b)) {
        GLog_Error("Failed to get model property for CSkeletalAnimatedObject");
        return;
    }
    if (!node.has(0x26fa86fe)) {
        GLog_Error("Failed to get skeleton property for CSkeletalAnimatedObject");
        return;
    }

    const auto &model_filename = node.get<std::string>(0x0f94740b);
    const auto &skeleton_filename = node.get<std::string>(0x26fa86fe);

    std::filesystem::path skeleton_bsk_name = skeleton_filename;
    skeleton_bsk_name.replace_extension(".bsk");

    const auto skeleton_node = export_file(app_state, hash_string(skeleton_bsk_name));
    const auto skin = helper.current_skin();

    if (!skin.is_valid()) {
        GLog_Error("Failed to get current skin for CSkeletalAnimatedObject");
        return;
    }
    const auto root_bone = helper.get<Node>(skin->joints[0]);
    // if (root_bone.is_valid()) {
    //     helper.set_parent(parent_gltf_node, root_bone);
    // }

    const auto output_node = export_adf_file(app_state, hash_string(model_filename));

    add_extras(node, output_node);
    set_world_matrix(output_node, node);
    if (parent_gltf_node.is_valid()) {
        helper.set_parent(parent_gltf_node, output_node);
    }
    else {
        GLog_Warning("Invalid parent setup: 0x%08X", node.name_hash());
    }

    process_children(app_state, node, path_hash, output_node);
    if (skeleton_node.is_valid()) {
        helper.pop_skin();
    }
}

void handle_CBoneAttachment(AppState &app_state, const RuntimeNode &node, const uint32 path_hash,
                            const GltfHelper::Handle<Node> &parent_gltf_node) {
    auto &helper = app_state.helper();

    std::string node_name;
    if (node.has("name")) {
        node_name = node.get<std::string>("name");
    }
    else {
        node_name = find_name(node.name_hash()).value_or(std::format("node_{:08X}", node.name_hash()));
    }

    auto output_node = helper.make<Node>();
    output_node->name = node_name;

    set_world_matrix(output_node, node);
    add_extras(node, output_node);

    const auto current_skin = helper.current_skin();
    if (current_skin.is_valid() && node.has(0x87becf63)) {
        const auto &parent_bone_name = node.get<std::string>(0x87becf63);
        const auto parent_bone = helper.find_node_in_skin(current_skin, parent_bone_name);
        if (parent_bone.is_valid()) {
            helper.set_parent(parent_bone, output_node);
        }
        else {
            GLog_Warning("Parent bone not found: {}", parent_bone_name);
        }
    }
    else if (parent_gltf_node.is_valid()) {
        helper.set_parent(parent_gltf_node, output_node);
    }
    else {
        GLog_Warning("Invalid parent setup: 0x%08X", node.name_hash());
    }

    process_children(app_state, node, path_hash, output_node);
}

void handle_default(AppState &app_state, const RuntimeNode &node, const uint32 path_hash,
                    const GltfHelper::Handle<Node> &parent_gltf_node) {
    auto &helper = app_state.helper();
    std::string node_name;
    if (node.has("name")) {
        if (node.is<std::string>("name")) {
            node_name = node.get<std::string>("name");
        }else {
            auto node_name_hash = node.get<uint32>("name");
            node_name = find_name(node_name_hash).value_or(std::format("node_{:08X}", node_name_hash));
        }
    }
    else {
        node_name = find_name(node.name_hash()).value_or(std::format("node_{:08X}", node.name_hash()));
    }

    auto output_node = helper.make<Node>();
    output_node->name = node_name;

    add_extras(node, output_node);
    set_world_matrix(output_node, node);

    if (parent_gltf_node.is_valid()) {
        helper.set_parent(parent_gltf_node, output_node);
    }
    else {
        GLog_Warning("Invalid parent setup: 0x%08X", node.name_hash());
    }

    process_children(app_state, node, path_hash, output_node);
}

void process_rtpc_node(AppState &app_state, const RuntimeNode &node, const uint32 path_hash,
                       const GltfHelper::Handle<Node> &parent_gltf_node) {
    ZoneScoped
    if (!node.has("_class")) {
        return;
    }
    const auto &class_name = node.get<std::string>("_class");

    if (class_name == "CCharacter") {
        handle_CCharacter(app_state, node, path_hash, parent_gltf_node);
    }
    else if (class_name == "CSecondaryMotionAttachment") {
        handle_CSecondaryMotionAttachment(app_state, node, path_hash, parent_gltf_node);
    }
    else if (class_name == "CRigidObject") {
        handle_CRigidObject(app_state, node, path_hash, parent_gltf_node);
    }
    else if (class_name == "CDamageableCharacterPart") {
        handle_CDamageableCharacterPart(app_state, node, path_hash, parent_gltf_node);
    }
    else if (class_name == "CSkeletalAnimatedObject") {
        handle_CSkeletalAnimatedObject(app_state, node, path_hash, parent_gltf_node);
    }
    else if (class_name == "CBoneAttachment") {
        handle_CBoneAttachment(app_state, node, path_hash, parent_gltf_node);
    }
    else {
        handle_default(app_state, node, path_hash, parent_gltf_node);
    }
}

GltfHelper::Handle<Node> export_rtpc(AppState &app_state, const std::unique_ptr<IO::File> &&buffer,
                                               const uint32 path_hash) {
    ZoneScoped
    auto &helper = app_state.helper();

    const RuntimeNode root_node = RuntimeNode::RootNode(buffer);

    // RuntimeNode_print(root_node, stdout, 0);
    // String epe_json = {};
    // String_init(&epe_json, 8192);
    // RuntimeNode_emit_json(root_node, &epe_json, 0);
    // printf("%s\n", String_data(&epe_json));

    const auto path = find_name(path_hash).value_or(std::format("path_{:08X}", path_hash));


    const auto epe_root_node = helper.make<Node>();
    epe_root_node->name = "epe_root";

    process_children(app_state, root_node, path_hash, epe_root_node);

    return epe_root_node;
}
