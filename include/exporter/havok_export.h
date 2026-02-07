// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_HAVOK_EXPORT_H
#define APEXPREDATOR_HAVOK_EXPORT_H
#include "cglm/cglm.h"
#include "havok/generated/havok_generated.h"
#include "platform/app_state.h"
#include "utils/gltf/cgltf_helper.h"

extern mat4 IDENTITY_MAT;

GL_ID export_havok_file(AppState* app_state, const TagFile *tag_file, StringView path);

void build_matrix(mat4 out, const hkQsTransform* transform);

GL_ID export_skeleton(AppState* app_state, const hkaSkeleton *skeleton);

void export_animation(AppState* app_state, const hkaAnimationBinding *binding, const hkaSkeleton* skeleton, StringView animation_name);
#endif //APEXPREDATOR_HAVOK_EXPORT_H