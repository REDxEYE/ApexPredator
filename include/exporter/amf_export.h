// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_AMF_EXPORT_H
#define APEXPREDATOR_AMF_EXPORT_H
#include "apex/adf/sti.h"
#include "apex/adf/adf_types.h"
#include "platform/app_state.h"
#include "utils/gltf/cgltf_helper.h"

GL_ID export_amf_mesh(AppState* app_state, uint32 path_hash, StringView path,
    const AmfMeshHeader *header, const AmfMeshBuffers *mesh_buffers);
GL_ID export_amf_model(AppState* app_state, const AmfModel *amf_model,
                       StringView path, uint32 path_hash);

#endif //APEXPREDATOR_AMF_EXPORT_H