// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_AMF_EXPORT_H
#define APEXPREDATOR_AMF_EXPORT_H
#include "apex/adf/sti.h"
#include "apex/adf/generated/adf_types.h"
#include "platform/app_state.h"

VM::NodePtr export_amf_mesh(ApexAppState& app_state, uint64 path_hash, const ADFTypes::AmfMeshHeader *header, const ADFTypes::AmfMeshBuffers *mesh_buffers);
VM::NodePtr export_amf_model(ApexAppState& app_state, const ADFTypes::AmfModel *amf_model, uint64 path_hash);

#endif //APEXPREDATOR_AMF_EXPORT_H