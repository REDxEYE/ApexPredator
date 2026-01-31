// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_HAVOK_EXPORT_H
#define APEXPREDATOR_HAVOK_EXPORT_H
#include "cglm/cglm.h"
#include "../havok/generated/havok_generated.h"
#include "utils/gltf/cgltf_helper.h"

extern mat4 IDENTITY_MAT;

GL_ID export_havok_file(GLTFContext *context, const TagFile *tag_file, const String* path, const String* export_path);

void build_matrix(mat4 out, const hkQsTransform* transform);

GL_ID export_skeleton(GLTFContext *context, const hkaSkeleton *skeleton);

void export_animation(GLTFContext *context, const hkaAnimationBinding *binding, const hkaSkeleton* skeleton, const String* name);
#endif //APEXPREDATOR_HAVOK_EXPORT_H