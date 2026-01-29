// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_HAVOK_EXPORT_H
#define APEXPREDATOR_HAVOK_EXPORT_H
#include "cglm/cglm.h"
#include "../havok/generated/havok_generated.h"
#include "utils/gltf/cgltf_helper.h"

extern mat4 IDENTITY_MAT;

void build_matrix(mat4 out, const hkQsTransform* transform);

GL_ID export_skeleton(GLTFContext *context, const hkaSkeleton *skeleton, Havok_TypeLibrary *havok_lib);

#endif //APEXPREDATOR_HAVOK_EXPORT_H