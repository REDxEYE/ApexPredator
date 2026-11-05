// Rage 2 and Second Extinction share hashed RTPC references and character nodes.
#include "exporter/rtpc_export.h"
#include <utility>

namespace RTPC {
SkeletonReference skeleton_reference(const RuntimeNode &node, const uint32 property,
                                     const std::string_view missing_name) {
    const uint32 hash = node.get<uint32>(property);
    if (missing_name.empty()) return {hash, {}};
    auto name = node.get_string(property);
    return {hash, name ? std::move(*name) : std::string(missing_name)};
}

uint32 model_reference(const RuntimeNode &node, const uint32 property) {
    return node.get<uint32>(property);
}

std::optional<VM::NodePtr> create_character_node(ApexAppState &app_state, const RuntimeNode &node,
                                                  const VM::NodePtr &parent_node) {
    auto &helper = app_state.models();
    auto output_node = helper.create_node();
    if (node.has("name"))
        output_node->name = node.get<std::string>("name");
    helper.set_parent(parent_node, output_node);
    return output_node;
}
} // namespace RTPC
