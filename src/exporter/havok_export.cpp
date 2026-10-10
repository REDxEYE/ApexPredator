// Created by RED on 12.01.2026.

#include "exporter/havok_export.h"

#include <fstream>
#include <string_view>
#include <ranges>

#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

#include "havok/animations/spline.h"
#include "redscore/utils/simple_fileio.h"


typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

using namespace std::string_view_literals;

auto IDENTITY_MAT = glm::identity<glm::mat4>();


void export_spline_compressed_animation(ApexAppState &app_state,
                                        const HavokTypes::hkaSplineCompressedAnimation *spline_animation,
                                        const HavokTypes::hkaAnimationBinding *binding,
                                        const HavokTypes::hkaSkeleton *skeleton,
                                        const std::string_view animation_name,
                                        const bool apply_root_motion) {
    auto &builder = app_state.models();
    const auto skin = builder.current_skin();
    if (!skin) throw std::runtime_error("Animation requires a skeleton");

    hkaSplineDecompressor decompressor{};
    decompressor.Assign(spline_animation);
    const float32 frame_duration = spline_animation->frameDuration;

    auto &animation = skin->animations.emplace_back();
    animation.name = animation_name;

    std::vector<float32> timestamps = {};

    const auto &extracted_motion = spline_animation->extractedMotion;
    const auto &ref_frame = Havok::as<HavokTypes::hkaDefaultAnimatedReferenceFrame>(extracted_motion);
    const hkArray<hkVector4f, HavokTypes::hkContainerHeapAllocator> *root_motion_frames = nullptr;
    if (ref_frame != nullptr) {
        root_motion_frames = &ref_frame->referenceFrameSamples;
    }


    timestamps.reserve(spline_animation->numFrames);

    for (int frame_id = 0; frame_id < spline_animation->numFrames; ++frame_id) {
        timestamps.push_back(frame_id * frame_duration);
    }

    for (int track_id = 0; track_id < binding->transformTrackToBoneIndices.size(); ++track_id) {
        std::vector<glm::vec3> positions = {};
        std::vector<glm::quat> rotations = {};
        std::vector<glm::vec3> scales = {};

        const uint32 bone_id = binding->transformTrackToBoneIndices[track_id];
        if ((track_id != 0 && bone_id == 0) || bone_id >= skeleton->bones.size()) {
            continue;
        }
        const HavokTypes::hkaBone &bone = skeleton->bones[bone_id];

        auto bone_node = builder.find_node_in_skin(skin, bone.name.stringAndFlag);

        if (!bone_node) {
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

            if (bone_id == 0 && apply_root_motion && root_motion_frames) {
                const auto &root_frame = root_motion_frames->at(frame_id);
                translation += root_frame;

                if (root_frame.value.x != 0.0f) {
                    rotation = glm::rotate(rotation, root_frame.value.w, glm::vec3(ref_frame->up.value));
                }

                // translation.y +=root_frame.value.z;
                // translation.y +=root_frame.value.w;
                // translation.z +=root_frame.value.y;
            }

            positions.emplace_back(translation);
            rotations.emplace_back(rotation);
            scales.emplace_back(scale);
        }


        VM::Channel position_channel, rotation_channel, scale_channel;
        position_channel.bone = rotation_channel.bone = scale_channel.bone = bone.name.stringAndFlag;
        position_channel.path = VM::AnimationPath::Translation;
        rotation_channel.path = VM::AnimationPath::Rotation;
        scale_channel.path = VM::AnimationPath::Scale;
        position_channel.times = rotation_channel.times = scale_channel.times = timestamps;
        for (const auto &v : positions) position_channel.values.insert(position_channel.values.end(), {v.x,v.y,v.z});
        for (const auto &v : rotations) rotation_channel.values.insert(rotation_channel.values.end(), {v.x,v.y,v.z,v.w});
        for (const auto &v : scales) scale_channel.values.insert(scale_channel.values.end(), {v.x,v.y,v.z});
        animation.channels.push_back(std::move(position_channel));
        animation.channels.push_back(std::move(rotation_channel));
        animation.channels.push_back(std::move(scale_channel));
    }
}

void export_animation(ApexAppState &app_state, const HavokTypes::hkaAnimationBinding *binding,
                      const HavokTypes::hkaSkeleton *skeleton,
                      const std::string_view animation_name,
                      const bool apply_root_motion) {
    export_skeleton(app_state, skeleton);

    if (const auto spline_compressed_animation = Havok::as<
        HavokTypes::hkaSplineCompressedAnimation>(binding->animation)) {
        export_spline_compressed_animation(app_state, spline_compressed_animation, binding, skeleton,
                                           animation_name, apply_root_motion);
    }
}

VM::NodePtr export_animation_container(ApexAppState &app_state,
                                                              const HavokTypes::hkaAnimationContainer *
                                                              animation_container) {
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

VM::NodePtr export_havok_file(ApexAppState &app_state,
                                                     std::unique_ptr<IO::File> &&buffer,
                                                     const std::string_view path) {
    Havok::Tag::TagFile tag_file(std::move(buffer));

    const auto item_obj = Havok::Tag::get_item(tag_file, 1);
    VM::NodePtr skeleton_id = {};

    if (const auto root_container = Havok::as<HavokTypes::hkRootLevelContainer>(item_obj)) {
        if (root_container->namedVariants.empty()) {
            throw std::runtime_error("No named variants in root container");
        }
        // if (root_container->namedVariants.size() > 1) {
        //     throw std::runtime_error("Multiple named variants in root container");
        // }
        for (const auto &[i, named_variant]: root_container->namedVariants|std::views::enumerate) {
            if (named_variant.className == "hkaAnimationContainer") {
                if (const auto animation_container = Havok::as<HavokTypes::hkaAnimationContainer>(named_variant.variant)) {
                    skeleton_id = export_animation_container(app_state, animation_container);
                }else {
                    throw std::runtime_error(std::format(
                        "Malformed hkaAnimationContainer, supposed to have hkaAnimationContainer, but had {}",
                        static_cast<std::string_view>(named_variant.className.stringAndFlag)));
                }
            } else {
                std::filesystem::path unk_file_export_path = app_state.export_path() / path;
                unk_file_export_path.replace_extension(std::format(".{}.{}.json", i,
                    static_cast<std::string_view>(named_variant.className.stringAndFlag)));
                std::filesystem::create_directories(unk_file_export_path.parent_path());

                std::ofstream json_out(unk_file_export_path);
                json_out << named_variant.to_json().dump(2);
                json_out.close();
            }
        }
    }
    if (skeleton_id) {
        return skeleton_id;
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

VM::NodePtr export_skeleton(ApexAppState &app_state, const HavokTypes::hkaSkeleton *skeleton) {
    if (!skeleton || skeleton->bones.size() != skeleton->parentIndices.size() ||
        skeleton->bones.size() != skeleton->referencePose.size())
        throw std::runtime_error("Invalid Havok skeleton arrays");
    auto model_skeleton = std::make_shared<VM::Skeleton>();
    model_skeleton->name = skeleton->name.stringAndFlag;
    for (size_t i = 0; i < skeleton->bones.size(); ++i) {
        const auto &pose = skeleton->referencePose[i];
        VM::Bone bone;
        bone.name = skeleton->bones[i].name.stringAndFlag;
        bone.parent = skeleton->parentIndices[i];
        bone.transform.translation = glm::vec3(pose.translation);
        bone.transform.rotation = glm::quat(pose.rotation.vec);
        bone.transform.scale = glm::vec3(pose.scale);
        model_skeleton->bones.push_back(std::move(bone));
    }
    return app_state.models().add_skeleton(std::move(model_skeleton))->root;
}
