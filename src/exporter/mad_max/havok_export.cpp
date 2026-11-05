#include "exporter/havok_export.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <utility>
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

VM::NodePtr export_havok_file(ApexAppState &app_state,
                              std::unique_ptr<IO::File> &&buffer,
                              const std::string_view path) {
    const auto skeletons = HavokTypes::read_skeletons(buffer->cbuffer());

    nlohmann::json result;
    if (skeletons.size() == 1) {
        result = skeletons.front().to_json();
    } else {
        result = nlohmann::json::array();
        result.get_ref<nlohmann::json::array_t &>().reserve(skeletons.size());
        for (const auto &skeleton: skeletons) result.push_back(skeleton.to_json());
    }

    auto save_path = app_state.export_path() / path;
    save_path.replace_extension("json");
    std::filesystem::create_directories(save_path.parent_path());
    std::ofstream out(save_path);
    if (!out) throw std::runtime_error("Failed to open Havok JSON: " + save_path.string());
    out << result.dump(2);
    if (!out) throw std::runtime_error("Failed to write Havok JSON: " + save_path.string());

    VM::NodePtr first_root;
    for (const auto &skeleton: skeletons) {
        auto root = export_skeleton(app_state, &skeleton);
        if (!first_root) first_root = std::move(root);
    }
    return first_root;
}

glm::mat4 build_matrix(const HavokTypes::hkQsTransform &transform) {
    auto out = glm::identity<glm::mat4>();
    const auto &translation = transform.translation.value;
    out = glm::translate(out, glm::vec3(translation[0], translation[1], translation[2]));
    const auto &rotation = transform.rotation.vec.value;
    out *= glm::mat4_cast(glm::quat(rotation[3], rotation[0], rotation[1], rotation[2]));
    const auto &scale = transform.scale.value;
    out = glm::scale(out, glm::vec3(scale[0], scale[1], scale[2]));
    return out;
}

VM::NodePtr export_skeleton(ApexAppState &app_state, const HavokTypes::hkaSkeleton *skeleton) {
    if (!skeleton || skeleton->bones.size() != skeleton->parentIndices.size() ||
        skeleton->bones.size() != skeleton->referencePose.size())
        throw std::runtime_error("Invalid Havok skeleton arrays");
    auto model_skeleton = std::make_shared<VM::Skeleton>();
    model_skeleton->name = skeleton->name.stringAndFlag;
    model_skeleton->bones.reserve(skeleton->bones.size());
    for (size_t i = 0; i < skeleton->bones.size(); ++i) {
        const auto &pose = skeleton->referencePose[i];
        VM::Bone bone;
        bone.name = skeleton->bones[i].name.stringAndFlag;
        bone.parent = skeleton->parentIndices[i];
        const auto &translation = pose.translation.value;
        bone.transform.translation = {translation[0], translation[1], translation[2]};
        const auto &rotation = pose.rotation.vec.value;
        bone.transform.rotation = {rotation[3], rotation[0], rotation[1], rotation[2]};
        const auto &scale = pose.scale.value;
        bone.transform.scale = {scale[0], scale[1], scale[2]};
        model_skeleton->bones.push_back(std::move(bone));
    }
    return app_state.models().add_skeleton(std::move(model_skeleton))->root;
}
