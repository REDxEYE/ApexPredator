// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_EPE_EXPORT_H
#define APEXPREDATOR_EPE_EXPORT_H
#include "apex/rtpc.h"
#include "apex/adf/sti.h"
#include "platform/app_state.h"
#include <optional>
#include <string>
#include <string_view>


// Linked once per RTPC-capable module; no generated game types cross this seam.
namespace RTPC {
    struct SkeletonReference {
        uint32 hash;
        std::string name;
    };

    SkeletonReference skeleton_reference(const RuntimeNode &node, uint32 property, std::string_view missing_name);
    uint32 model_reference(const RuntimeNode &node, uint32 property);
    std::optional<VM::NodePtr> create_character_node(ApexAppState &app_state, const RuntimeNode &node,
                                                      const VM::NodePtr &parent_node);
}

void add_extras(const RuntimeNode &node, const VM::NodePtr &output_node);
void set_world_matrix(const VM::NodePtr &model_node, const RuntimeNode &node);

void process_rtpc_node(ApexAppState &app_state, const RuntimeNode &node, uint64 path_hash,
                       const VM::NodePtr &parent_gltf_node);

VM::NodePtr export_rtpc(ApexAppState& app_state, const std::unique_ptr<IO::File> &&buffer, uint64 path_hash);


#endif //APEXPREDATOR_EPE_EXPORT_H
