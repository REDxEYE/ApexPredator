// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_EPE_EXPORT_H
#define APEXPREDATOR_EPE_EXPORT_H
#include "apex/rtpc.h"
#include "apex/adf/sti.h"
#include "platform/app_state.h"


void process_rtpc_node(ApexAppState &app_state, const RuntimeNode &node, uint32 path_hash,
                       const GltfHelper::Handle<tinygltf::Node> &parent_gltf_node);

GltfHelper::Handle<tinygltf::Node> export_rtpc(ApexAppState& app_state, const std::unique_ptr<IO::File> &&buffer, uint32 path_hash);


#endif //APEXPREDATOR_EPE_EXPORT_H
