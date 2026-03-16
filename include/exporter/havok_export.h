// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_HAVOK_EXPORT_H
#define APEXPREDATOR_HAVOK_EXPORT_H
#include "glm/glm.hpp"
#include "havok/generated/havok_types.h"
#include "platform/app_state.h"

#define HAVOK_MAGIC  "TAG0"

// extern mat4 IDENTITY_MAT;

GltfHelper::Handle<tinygltf::Node> export_havok_file(AppState& app_state, std::unique_ptr<IO::File> && buffer, std::string_view path);

glm::mat4 build_matrix(const HavokTypes::hkQsTransform& transform);

GltfHelper::Handle<tinygltf::Node> export_skeleton(AppState& app_state, const HavokTypes::hkaSkeleton *skeleton);

void export_animation(AppState& app_state, const HavokTypes::hkaAnimationBinding *binding, const HavokTypes::hkaSkeleton* skeleton, std::string_view animation_name);
#endif //APEXPREDATOR_HAVOK_EXPORT_H