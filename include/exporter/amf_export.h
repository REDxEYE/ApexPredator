// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_AMF_EXPORT_H
#define APEXPREDATOR_AMF_EXPORT_H
#include "apex/adf/sti.h"
#include "apex/adf/generated/adf_types.h"
#include "platform/app_state.h"

GltfHelper::Handle<tinygltf::Node> export_amf_mesh(ApexAppState& app_state, uint32 path_hash, const ADFTypes::AmfMeshHeader *header, const ADFTypes::AmfMeshBuffers *mesh_buffers);
GltfHelper::Handle<tinygltf::Node> export_amf_model(ApexAppState& app_state, const ADFTypes::AmfModel *amf_model, uint32 path_hash);

#endif //APEXPREDATOR_AMF_EXPORT_H