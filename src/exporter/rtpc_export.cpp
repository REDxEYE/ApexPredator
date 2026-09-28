// Created by RED on 12.01.2026.

#include "exporter/rtpc_export.h"
#include "games.hpp"
#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"

#include "apex/hashes.h"
#include "exporter/havok_export.h"
#include "exporter/adf_export.h"
#include "exporter/common_export.h"
#include "redscore/platform/logger.h"
#include "tracy/Tracy.hpp"

void add_extras(const RuntimeNode &node, const VM::NodePtr &output_node) {
    if (output_node) output_node->extras = node.to_json();
}

glm::mat4 calculate_global_node_matrix(VM::SceneBuilder &, const VM::NodePtr &target) {
    return VM::SceneBuilder::global_matrix(target);
}

void process_children(ApexAppState &app_state, const RuntimeNode &node, const uint64 path_hash,
                      const VM::NodePtr &parent_node) {
    for (const auto &child: node.children()) {
        process_rtpc_node(app_state, child, path_hash, parent_node);
    }
}

void set_world_matrix(const VM::NodePtr &model_node, const RuntimeNode &node) {
    if (!model_node || !node.has("world"))
        return;
    const auto &matrix = node.get<glm::mat4>("world");
    if (matrix != glm::identity<glm::mat4>())
        model_node->transform.matrix_override = matrix;
}

void handle_CCharacter(ApexAppState &app_state,
                       const RuntimeNode &node, const uint64 path_hash,
                       const VM::NodePtr &parent_node) {
    VM::SceneBuilder &helper = app_state.models();

    if (!node.has("skeleton")) {
        std::cout << node.to_json() << std::endl;
        GLog_Error("Failed to get skeleton property for CCharacter");
        return;
    }
#if GAME==GAME_GENERATION_ZERO
    const auto &skeleton_filename = node.get<std::string>("skeleton");
    std::filesystem::path skeleton_bsk_name = skeleton_filename;
    skeleton_bsk_name.replace_extension(".bsk");
    const std::string skeleton_name = skeleton_bsk_name.generic_string();
    const uint32 skeleton_path_hash = hash_string(skeleton_bsk_name);
#elif GAME==GAME_RAGE2
    const uint32 skeleton_path_hash = node.get<uint32>("skeleton");
    const std::string skeleton_name = node.get_string("skeleton").value_or("<no path>");
#else
#error "Unsupported game"
#endif

    const auto skeleton_node = export_file(app_state, skeleton_path_hash);

    if (!skeleton_node) {
        GLog_Error("Failed to export skeleton for CCharacter: {}", skeleton_name);
        throw std::runtime_error("Failed to export skeleton for CCharacter");
    }

    const auto skin = helper.current_skin();
    if (!skin) {
        GLog_Error("Failed to get current skin for CCharacter");
        throw std::runtime_error("Failed to get current skin for CCharacter");
    }

    const auto root_bone = skin->joints.at(0);
    if (!root_bone) {
        GLog_Error("Failed to get root bone for CCharacter");
        throw std::runtime_error("Failed to get root bone for CCharacter");
    }

#if GAME==GAME_GENERATION_ZERO

    if (!node.has(0xE8129FE6)) {
        std::cout << node.to_json() << std::endl;
        GLog_Error("Failed to get model property for CCharacter");
        return;
    }

    const auto &model_filename = node.get<std::string>(0xE8129FE6);

    // helper.set_parent(parent_node, root_bone);

    const auto output_node = export_adf_file(app_state, hash_string(model_filename));

    add_extras(node, output_node);
    set_world_matrix(output_node, node);
    if (parent_node)
        helper.set_parent(parent_node, output_node);
    else {
        GLog_Warning("Invalid parent setup: 0x{:08X}", node.name_hash());
    }
#else
    const auto output_node = helper.create_node();
    if (node.has("name"))
        output_node->name = node.get<std::string>("name");
    helper.set_parent(parent_node, output_node);
#endif

    process_children(app_state, node, path_hash, output_node);
    if (skin) {
        helper.pop_skin();
    }
}

void handle_SCharacterPart(ApexAppState &app_state,
                           const RuntimeNode &node, const uint64 path_hash,
                           const VM::NodePtr &parent_node) {
    auto &helper = app_state.models();
    if (!node.has(3029910141)) {
        GLog_Error("Failed to get model property for SCharacterPart");
        return;
    }
    const auto model_path = node.get<uint32>(3029910141);
    const auto output_node = export_adf_file(app_state, model_path);
    set_world_matrix(output_node, node);
    add_extras(node, output_node);
    if (parent_node)
        helper.set_parent(parent_node, output_node);
    else {
        GLog_Warning("Invalid parent setup: 0x{:08X}", node.name_hash());
    }
    process_children(app_state, node, path_hash, output_node);
}

void handle_CSecondaryMotionAttachment(ApexAppState &app_state,
                                       const RuntimeNode &node, const uint64 path_hash,
                                       const VM::NodePtr &parent_node) {
    auto &helper = app_state.models();
    if (!node.has("model")) {
        GLog_Error("Failed to get model property for CSecondaryMotionAttachment");
        return;
    }
    if (!node.has("skeleton")) {
        GLog_Error("Failed to get skeleton property for CSecondaryMotionAttachment");
        return;
    }
    const auto &model_filename = node.get<std::string>("model");
    const auto &skeleton_filename = node.get<std::string>("skeleton");
    std::filesystem::path skeleton_bsk_name = skeleton_filename;
    skeleton_bsk_name.replace_extension(".bsk");
    const auto skeleton_node = export_file(app_state, hash_string(skeleton_bsk_name));
    if (!skeleton_node) {
        GLog_Error("Failed to export skeleton for CSecondaryMotionAttachment: {}", skeleton_bsk_name.string());
        throw std::runtime_error("Failed to export skeleton for CSecondaryMotionAttachment");
    }
    const auto skin = helper.current_skin();
    if (!skin) {
        GLog_Error("Failed to get current skin for CSecondaryMotionAttachment");
    }

    const auto output_node = export_adf_file(app_state, hash_string(model_filename));
    set_world_matrix(output_node, node);
    add_extras(node, output_node);
    if (parent_node)
        helper.set_parent(parent_node, output_node);
    else {
        GLog_Warning("Invalid parent setup: 0x{:08X}", node.name_hash());
    }
    process_children(app_state, node, path_hash, output_node);
    if (skin) {
        helper.pop_skin();
    }
}

void handle_CDamageableCharacterPart(ApexAppState &app_state,
                                     const RuntimeNode &node, const uint64 path_hash,
                                     const VM::NodePtr &parent_node) {
    auto &helper = app_state.models();
    std::string node_name = {};
    if (node.has("name"))
        node_name = node.get<std::string>("name");
    else
        node_name = find_lookup3_name(node.name_hash()).value_or(std::format("node_{:08X}", node.name_hash()));

    const auto output_node = helper.create_node();
    output_node->name = node_name;

    set_world_matrix(output_node, node);
    add_extras(node, output_node);

    const auto current_skin = helper.current_skin();
    if (current_skin && node.has(0x4d67eec5)) {
        const auto &parent_bone_name = node.get<std::string>(0x4d67eec5);
        const auto parent_bone = helper.find_node_in_skin(current_skin, parent_bone_name);
        if (parent_bone) {
            glm::mat4 node_global_matrix = calculate_global_node_matrix(helper, parent_bone);
            node_global_matrix = glm::inverse(node_global_matrix);
            output_node->transform.matrix_override = node_global_matrix;
            helper.set_parent(parent_bone, output_node);
        } else {
            GLog_Warning("Parent bone not found: {}", parent_bone_name);
        }
    } else if (parent_node) {
        helper.set_parent(parent_node, output_node);
    } else {
        GLog_Warning("Invalid parent setup: {}", node.name_hash());
    }

    process_children(app_state, node, path_hash, output_node);
}

void handle_CRigidObject(ApexAppState &app_state, const RuntimeNode &node, const uint64 path_hash,
                         const VM::NodePtr &parent_node) {
    auto &helper = app_state.models();

    const auto model_filename_hash = node.get<uint32>("filename");
    if (model_filename_hash == 0) {
        GLog_Error("Failed to get model property for CRigidObject");
        return;
    }
    auto output_node = export_adf_file(app_state, model_filename_hash);
    if (output_node) {
        set_world_matrix(output_node, node);

        if (parent_node)
            helper.set_parent(parent_node, output_node);
        else {
            GLog_Warning("Invalid parent setup: 0x{:08X}", node.name_hash());
        }
    } else {
        const auto model_filename = find_name(model_filename_hash).or_else([&] {
            return find_lookup3_name(node.name_hash());
        }).value_or(std::format("model_{:08X}", model_filename_hash));

        output_node = helper.create_node();
        output_node->name = model_filename;
        set_world_matrix(output_node, node);
        add_extras(node, output_node);
    }
    process_children(app_state, node, path_hash, output_node);
}

void handle_CSkeletalAnimatedObject(ApexAppState &app_state, const RuntimeNode &node, const uint64 path_hash,
                                    const VM::NodePtr &parent_node) {
    auto &helper = app_state.models();

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

    if (!skin) {
        GLog_Error("Failed to get current skin for CSkeletalAnimatedObject");
        return;
    }
    const auto root_bone = skin->joints.at(0);
    // if (root_bone) {
    //     helper.set_parent(parent_node, root_bone);
    // }

    const auto output_node = export_adf_file(app_state, hash_string(model_filename));

    add_extras(node, output_node);
    set_world_matrix(output_node, node);
    if (parent_node) {
        helper.set_parent(parent_node, output_node);
    } else {
        GLog_Warning("Invalid parent setup: 0x%08X", node.name_hash());
    }

    process_children(app_state, node, path_hash, output_node);
    if (skeleton_node) {
        helper.pop_skin();
    }
}

void handle_CBoneAttachment(ApexAppState &app_state, const RuntimeNode &node, const uint64 path_hash,
                            const VM::NodePtr &parent_node) {
    auto &helper = app_state.models();

    std::string node_name;
    if (node.has("name")) {
        node_name = node.get<std::string>("name");
        node_name = "<Attachment> " + node_name;
    } else {
        node_name = find_lookup3_name(node.name_hash()).value_or(std::format("node_{:08X}", node.name_hash()));
    }

    auto output_node = helper.create_node();
    output_node->name = node_name;

    set_world_matrix(output_node, node);
    add_extras(node, output_node);

    const auto current_skin = helper.current_skin();
    if (current_skin && node.has(0x87becf63)) {
        const auto &parent_bone_name = node.get<std::string>(0x87becf63);
        const auto parent_bone = helper.find_node_in_skin(current_skin, parent_bone_name);
        if (parent_bone) {
            helper.set_parent(parent_bone, output_node);
        } else {
            GLog_Warning("Parent bone not found: {}", parent_bone_name);
        }
    } else if (parent_node) {
        helper.set_parent(parent_node, output_node);
    } else {
        GLog_Warning("Invalid parent setup: 0x%08X", node.name_hash());
    }

    process_children(app_state, node, path_hash, output_node);
}

void handle_default(ApexAppState &app_state, const RuntimeNode &node, const uint64 path_hash,
                    const VM::NodePtr &parent_node) {
    auto &helper = app_state.models();
    std::string node_name;
    if (node.has("name")) {
        if (node.is<std::string>("name")) {
            node_name = node.get<std::string>("name");
        } else {
            auto node_name_hash = node.get<uint32>("name");
            node_name = find_lookup3_name(node_name_hash).value_or(std::format("node_{:08X}", node_name_hash));
        }
    } else {
        node_name = find_lookup3_name(node.name_hash()).value_or(std::format("node_{:08X}", node.name_hash()));
    }

    auto output_node = helper.create_node();
    output_node->name = node_name;

    add_extras(node, output_node);
    set_world_matrix(output_node, node);

    if (parent_node) {
        helper.set_parent(parent_node, output_node);
    } else {
        GLog_Warning("Invalid parent setup: 0x%08X", node.name_hash());
    }

    process_children(app_state, node, path_hash, output_node);
}

void process_rtpc_node(ApexAppState &app_state, const RuntimeNode &node, const uint64 path_hash,
                       const VM::NodePtr &parent_node) {
    ZoneScoped
    if (!(node.has("_class") || node.has("_class_hash"))) {
        return;
    }
    std::string class_name;
    if (node.has("_class")) {
        class_name = node.get<std::string>("_class");
    } else if (node.has("_class_hash")) {
        auto opt_name = find_lookup3_name(node.get<uint32>("_class_hash"));
        if (!opt_name) {
            return;
        }
        class_name = opt_name.value();
    } else {
        return;
    }


    if (class_name == "CCharacter") {
        handle_CCharacter(app_state, node, path_hash, parent_node);
    } else if (class_name == "SCharacterPart") {
        handle_SCharacterPart(app_state, node, path_hash, parent_node);
    } else if (class_name == "CSecondaryMotionAttachment") {
        handle_CSecondaryMotionAttachment(app_state, node, path_hash, parent_node);
    } else if (class_name == "CRigidObject") {
        handle_CRigidObject(app_state, node, path_hash, parent_node);
    } else if (class_name == "CDamageableCharacterPart") {
        handle_CDamageableCharacterPart(app_state, node, path_hash, parent_node);
    } else if (class_name == "CSkeletalAnimatedObject") {
        handle_CSkeletalAnimatedObject(app_state, node, path_hash, parent_node);
    } else if (class_name == "CBoneAttachment") {
        handle_CBoneAttachment(app_state, node, path_hash, parent_node);
    } else {
        handle_default(app_state, node, path_hash, parent_node);
    }
}

VM::NodePtr export_rtpc(ApexAppState &app_state, const std::unique_ptr<IO::File> &&buffer,
                        const uint64 path_hash) {
    ZoneScoped
    auto &helper = app_state.models();

    const RuntimeNode root_node = RuntimeNode::RootNode(buffer);

    const auto path = find_name(path_hash).value_or(std::format("path_{:08X}", path_hash));

    auto export_path = app_state.export_path();
    auto file_export_path = export_path / (path + ".json");
    auto json_data = root_node.to_json();
    std::ofstream file(file_export_path);
    file << json_data.dump(2);
    file.close();


    const auto epe_root_node = helper.create_node();
    epe_root_node->name = "epe_root";

    process_children(app_state, root_node, path_hash, epe_root_node);

    return epe_root_node;
}
