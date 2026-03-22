// Created by RED on 12.01.2026.

#include "exporter/havok_export.h"

#include <string_view>
#include <ranges>

#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

#include "havok/animations/spline.h"


typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

using namespace std::string_view_literals;

auto IDENTITY_MAT = glm::identity<glm::mat4>();


void export_spline_compressed_animation(AppState &app_state,
                                        const HavokTypes::hkaSplineCompressedAnimation *spline_animation,
                                        const HavokTypes::hkaAnimationBinding *binding,
                                        const HavokTypes::hkaSkeleton *skeleton,
                                        const std::string_view animation_name) {
    GltfHelper &gltf_helper = app_state.helper();

    hkaSplineDecompressor decompressor{};
    decompressor.Assign(spline_animation);
    const float32 frame_duration = spline_animation->frameDuration;

    const auto animation = gltf_helper.make<tinygltf::Animation>();
    animation->name = animation_name;

    std::vector<float32> timestamps = {};


    timestamps.reserve(spline_animation->numFrames);

    for (int frame_id = 0; frame_id < spline_animation->numFrames; ++frame_id) {
        timestamps.push_back(frame_id * frame_duration);
    }

    const auto timestamps_accessor = gltf_helper.create_accessor_chain_from_u8(
        reinterpret_cast<const uint8 *>(timestamps.data()), timestamps.size() * sizeof(float32), 0,
        TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_SCALAR, timestamps.size(), false, 0, 0,
        "spline_animation_timestamps");

    timestamps_accessor.accessor->minValues = {0};
    timestamps_accessor.accessor->maxValues = {spline_animation->duration};

    // if (skeleton->bones.size() != binding->transformTrackToBoneIndices.size()) {
    //     GLog_Warning(
    //         "Number of transform tracks in the animation binding does not match the number of bones in the skeleton. Some tracks will be skipped. {} in binding vs {} in animation",
    //         binding->transformTrackToBoneIndices.size(), skeleton->bones.size());
    // }

    for (int track_id = 0; track_id < binding->transformTrackToBoneIndices.size(); ++track_id) {
        std::vector<glm::vec3> positions = {};
        std::vector<glm::quat> rotations = {};
        std::vector<glm::vec3> scales = {};

        const uint32 bone_id = binding->transformTrackToBoneIndices[track_id];
        if ((track_id != 0 && bone_id == 0) || bone_id >= skeleton->bones.size()) {
            continue;
        }
        const HavokTypes::hkaBone &bone = skeleton->bones[bone_id];

        auto bone_node = gltf_helper.find<tinygltf::Node>(bone.name.stringAndFlag);

        if (!bone_node.is_valid()) {
            continue;
        }

        positions.reserve(spline_animation->numFrames);
        rotations.reserve(spline_animation->numFrames);
        scales.reserve(spline_animation->numFrames);

        for (int frame_id = 0; frame_id < spline_animation->numFrames; ++frame_id) {
            uint32 block_id = frame_id / spline_animation->maxFramesPerBlock;

            if (block_id >= decompressor.blocks.size()) {
                block_id = decompressor.blocks.size() - 1;
            }
            uint32 local_frame = frame_id % spline_animation->maxFramesPerBlock;

            const TransformSplineBlock *block = &decompressor.blocks[block_id];
            auto [translation, rotation, scale] = block->GetValue(track_id, local_frame);

            positions.emplace_back(translation);
            rotations.emplace_back(rotation);
            scales.emplace_back(scale);
        }


        const auto position_accessor = gltf_helper.create_accessor_chain_from_u8(
            reinterpret_cast<uint8 *>(positions.data()), positions.size() * 3 * sizeof(float32), 0,
            TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC3, positions.size(), false,
            0, 0, "spline_animation_positions"
        );

        const auto rotation_accessor = gltf_helper.create_accessor_chain_from_u8(
            reinterpret_cast<uint8 *>(rotations.data()), rotations.size() * 4 * sizeof(float32), 0,
            TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC4, rotations.size(), false,
            0, 0, "spline_animation_rotations"
        );

        const auto scale_accessor = gltf_helper.create_accessor_chain_from_u8(
            reinterpret_cast<uint8 *>(scales.data()), scales.size() * 3 * sizeof(float32), 0,
            TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC3, scales.size(), false,
            0, 0, "spline_animation_scales"
        );


        auto &position_sampler = animation->samplers.emplace_back();
        position_sampler.input = timestamps_accessor.accessor.index();
        position_sampler.interpolation = "LINEAR";
        position_sampler.output = position_accessor.accessor.index();

        auto &position_channel = animation->channels.emplace_back();
        position_channel.sampler = animation->samplers.size() - 1;
        position_channel.target_node = bone_node.index();
        position_channel.target_path = "translation";

        auto &rotation_sampler = animation->samplers.emplace_back();
        rotation_sampler.input = timestamps_accessor.accessor.index();
        rotation_sampler.interpolation = "LINEAR";
        rotation_sampler.output = rotation_accessor.accessor.index();

        auto &rotation_channel = animation->channels.emplace_back();
        rotation_channel.sampler = animation->samplers.size() - 1;
        rotation_channel.target_node = bone_node.index();
        rotation_channel.target_path = "rotation";

        auto &scale_sampler = animation->samplers.emplace_back();
        scale_sampler.input = timestamps_accessor.accessor.index();
        scale_sampler.interpolation = "LINEAR";
        scale_sampler.output = scale_accessor.accessor.index();

        auto &scale_channel = animation->channels.emplace_back();
        scale_channel.sampler = animation->samplers.size() - 1;
        scale_channel.target_node = bone_node.index();
        scale_channel.target_path = "scale";
    }
}

void export_animation(AppState &app_state, const HavokTypes::hkaAnimationBinding *binding,
                      const HavokTypes::hkaSkeleton *skeleton,
                      const std::string_view animation_name) {
    export_skeleton(app_state, skeleton);

    if (const auto spline_compressed_animation = Havok::as<
        HavokTypes::hkaSplineCompressedAnimation>(binding->animation)) {
        export_spline_compressed_animation(app_state, spline_compressed_animation, binding, skeleton,
                                           animation_name);
    }
}

GltfHelper::Handle<tinygltf::Node> export_animation_container(AppState &app_state,
                                                              const HavokTypes::hkaAnimationContainer *
                                                              animation_container) {
    GltfHelper::Handle<tinygltf::Skin> skeleton_id = {};
    for (int i = 0; i < animation_container->skeletons.size(); ++i) {
        const auto &skeleton = animation_container->skeletons[i];
        return export_skeleton(app_state, skeleton.get());
    }
    // for (int i = 0; i < animation_container->bindings.size(); ++i) {
    //     const auto *binding = animation_container->bindings[i].get();
    //     export_animation(app_state, binding);
    // }
    return {};
}

GltfHelper::Handle<tinygltf::Node> export_havok_file(AppState &app_state,
                                                     std::unique_ptr<IO::File> &&buffer,
                                                     const std::string_view path) {
    Havok::Tag::TagFile tag_file(std::move(buffer));

    const auto item_obj = Havok::Tag::get_item(tag_file, 1);

    if (const auto root_container = Havok::as<HavokTypes::hkRootLevelContainer>(item_obj)) {
        if (root_container->namedVariants.empty()) {
            throw std::runtime_error("No named variants in root container");
        }
        if (root_container->namedVariants.size() > 1) {
            throw std::runtime_error("Multiple named variants in root container");
        }
        const auto &named_variant = root_container->namedVariants.front();
        if (named_variant.className == "hkaAnimationContainer") {
            if (const auto animation_container = Havok::as<HavokTypes::hkaAnimationContainer>(named_variant.variant)) {
                return export_animation_container(app_state, animation_container);
            }
            const auto &ti = typeid(*named_variant.variant);
            throw std::runtime_error(std::format(
                "Malformed hkaAnimationContainer, supposed to have hkaAnimationContainer, but had {}", ti.name()));
        }
    }
    return {};
}

glm::mat4 build_matrix(const HavokTypes::hkQsTransform &transform) {
    auto out = glm::identity<glm::mat4>();
    out = glm::translate(out, glm::vec3(transform.translation));
    out *= glm::mat4_cast(glm::quat(transform.rotation.vec));
    out = glm::scale(out, glm::vec3(transform.scale));
    return out;
}

GltfHelper::Handle<tinygltf::Node> export_skeleton(AppState &app_state,
                                                   const HavokTypes::hkaSkeleton *skeleton) {
    GltfHelper &helper = app_state.helper();

    const auto skeleton_node = helper.make<tinygltf::Node>();
    skeleton_node->name = skeleton->name.stringAndFlag;
    skeleton_node->name += "_Skeleton";
    const auto skin = helper.make<tinygltf::Skin>();
    skin->name = skeleton->name.stringAndFlag;
    helper.add_to_scene(skeleton_node);
    skin->skeleton = skeleton_node.index();

    std::vector<GltfHelper::Handle<tinygltf::Node> > bones = {};
    std::vector<glm::mat4> inverse_matrices = {};
    std::vector<glm::mat4> global_matrices = {};
    bones.reserve(skeleton->bones.size());
    inverse_matrices.reserve(skeleton->bones.size());
    global_matrices.reserve(skeleton->bones.size());

    for (const auto [bone_id, bone]: skeleton->bones | std::views::enumerate) {
        const auto bone_node = helper.make<tinygltf::Node>();
        bone_node->name = bone.name.stringAndFlag;
        bones.emplace_back(bone_node);
        skin->joints.emplace_back(bone_node.index());

        const int16 bone_parent_id = skeleton->parentIndices[bone_id];
        if (bone_parent_id >= 0) {
            bones[bone_parent_id]->children.push_back(bone_node.index());
        }
        else {
            skeleton_node->children.push_back(bone_node.index());
        }
        const HavokTypes::hkQsTransform &transform = skeleton->referencePose[bone_id];
        glm::mat4 bone_matrix = build_matrix(transform);
        global_matrices.emplace_back(bone_matrix);

        if (bone_parent_id >= 0) {
            global_matrices[bone_id] = global_matrices[bone_parent_id] * bone_matrix;
        }

        if (bone_matrix != glm::identity<glm::mat4>()) {
            GltfHelper::set_node_transform(*bone_node, transform.translation, transform.scale,
                                           glm::quat(transform.rotation.vec));
        }
    }
    for (int i = 0; i < skeleton->bones.size(); ++i) {
        glm::mat4 inverse_matrix = glm::inverse(global_matrices[i]);
        inverse_matrices.push_back(inverse_matrix);
    }
    auto accessor = helper.create_accessor_chain_from_u8(reinterpret_cast<uint8 *>(inverse_matrices.data()),
                                                         inverse_matrices.size() * 16 * sizeof(float32), 0,
                                                         TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_MAT4,
                                                         inverse_matrices.size(), false, 0, 0,
                                                         "inverse_matrices"
    );

    skin->inverseBindMatrices = accessor.accessor.index();
    helper.push_skin(skin);
    return skeleton_node;
}
