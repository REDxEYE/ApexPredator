#include "exporter/rtpc_export.h"

#include "apex/hashes.h"
#include "exporter/adf_export.h"
#include "redscore/platform/logger.h"

#include <filesystem>
#include <iostream>

namespace RTPC {
SkeletonReference skeleton_reference(const RuntimeNode &node, const uint32 property,
                                     const std::string_view missing_name) {
    std::filesystem::path bsk_name = node.get<std::string>(property);
    bsk_name.replace_extension(".bsk");
    const auto hash = static_cast<uint32>(hash_string(bsk_name));
    return {hash, missing_name.empty() ? std::string{} : bsk_name.generic_string()};
}

uint32 model_reference(const RuntimeNode &node, const uint32 property) {
    return static_cast<uint32>(hash_string(node.get<std::string>(property)));
}

std::optional<VM::NodePtr> create_character_node(ApexAppState &app_state, const RuntimeNode &node,
                                                  const VM::NodePtr &parent_node) {
    constexpr uint32 model_property = 0xE8129FE6;
    if (!node.has(model_property)) {
        std::cout << node.to_json() << std::endl;
        GLog_Error("Failed to get model property for CCharacter");
        return std::nullopt;
    }

    auto output_node = export_adf_file(app_state, model_reference(node, model_property));
    add_extras(node, output_node);
    set_world_matrix(output_node, node);
    if (parent_node)
        app_state.models().set_parent(parent_node, output_node);
    else
        GLog_Warning("Invalid parent setup: 0x{:08X}", node.name_hash());
    return output_node;
}
} // namespace RTPC
