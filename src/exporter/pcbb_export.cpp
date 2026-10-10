#include "exporter/pcbb_export.h"

#include "apex/hashes.h"
#include "apex/rbmdl/rbmdl_file.hpp"
#include "redscore/platform/logger.h"
#include "tracy/Tracy.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace {
using Apex::PCBB::Property;
using Apex::PCBB::PropertyKey;
using Apex::PCBB::PropertySection;

std::string lower_basename(std::string_view path) {
    const auto separator = path.find_last_of("/\\");
    std::string name(path.substr(separator == std::string_view::npos ? 0 : separator + 1));
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return name;
}

class PcbbSceneExporter {
public:
    explicit PcbbSceneExporter(ApexAppState &app_state) : app_(app_state), builder_(app_state.models()) {}

    VM::NodePtr build(const Apex::PCBB::PropertyFile &file) {
        const auto root = builder_.create_node("epe_root");
        for (std::size_t index = 0; index < file.sections.size(); ++index) {
            const auto &section = file.sections[index];
            const auto section_node = create_node(section, root, index);
            process_groups(section.properties, section_node);
        }
        return root;
    }

private:
    template<typename Container>
    static std::string node_name(const Container &source, std::size_t index) {
        if (source.has(PropertyKey::Name)) {
            if (auto name = source.get_string(PropertyKey::Name); name && !name->empty()) return std::move(*name);
        }
        if (source.has(PropertyKey::Class)) {
            if (auto name = source.get_string(PropertyKey::Class); name && !name->empty()) return std::move(*name);
        }
        if constexpr (std::is_same_v<Container, Property>) {
            if (auto name = find_lookup3_name(source.name_hash)) return std::move(*name);
            return std::format("group_{:08X}", source.name_hash);
        } else {
            return std::format("section_{}", index);
        }
    }

    std::optional<uint32> lod_mesh_hash(uint32 lod_hash) {
        if (const auto it = lod_targets_.find(lod_hash); it != lod_targets_.end()) return it->second;
        const auto lod = app_.manager().get(lod_hash);
        if (!lod) {
            GLog_Warning("PCBB LOD resource not found: 0x{:08X}", lod_hash);
            return lod_targets_.emplace(lod_hash, std::nullopt).first->second;
        }
        std::string manifest(lod->get_size(), '\0');
        lod->read(manifest.data(), manifest.size());
        const auto first_line = std::string_view(manifest).substr(0, manifest.find_first_of("\r\n"));
        const auto mesh_name = lower_basename(first_line);
        if (!mesh_name.ends_with(".rbm")) {
            throw std::runtime_error(std::format("PCBB LOD has no leading RBM path: 0x{:08X}", lod_hash));
        }
        return lod_targets_.emplace(lod_hash, Apex::PCBB::pcbb_name_hash(mesh_name)).first->second;
    }

    template<typename Container>
    VM::NodePtr load_model(const Container &source) {
        const Property *resource = source.find(PropertyKey::Filename);
        if (!resource) resource = source.find(PropertyKey::ResourceFile);
        if (!resource) return {};
        const auto *path = std::get_if<std::string>(&resource->value);
        if (!path || path->empty()) return {};
        const auto asset_name = lower_basename(*path);
        std::optional<uint32> mesh_hash;
        if (asset_name.ends_with(".lod")) {
            mesh_hash = lod_mesh_hash(Apex::PCBB::pcbb_name_hash(asset_name));
        } else if (asset_name.ends_with(".rbm")) {
            mesh_hash = Apex::PCBB::pcbb_name_hash(asset_name);
        }
        if (!mesh_hash) return {};

        if (const auto it = models_.find(*mesh_hash); it != models_.end()) {
            if (!it->second) return {};
            auto node = builder_.create_node();
            node->model = it->second;
            return node;
        }
        auto buffer = app_.manager().get(*mesh_hash);
        if (!buffer) {
            GLog_Warning("PCBB mesh resource not found: 0x{:08X}", *mesh_hash);
            models_.emplace(*mesh_hash, nullptr);
            return {};
        }
        auto node = Apex::export_rbmdl(app_, *mesh_hash, std::move(buffer));
        models_.emplace(*mesh_hash, node->model);
        return node;
    }

    template<typename Container>
    VM::NodePtr create_node(const Container &source, const VM::NodePtr &parent, std::size_t index = 0) {
        auto node = load_model(source);
        if (!node) node = builder_.create_node();
        node->name = node_name(source, index);
        node->extras = source.to_json();
        if (const auto *world = source.find("world")) {
            if (const auto *matrix = std::get_if<glm::mat4>(&world->value);
                matrix && *matrix != glm::identity<glm::mat4>()) {
                node->transform.matrix_override = *matrix;
            }
        }
        builder_.set_parent(parent, node);
        return node;
    }

    void process_groups(const std::vector<Property> &properties, const VM::NodePtr &parent) {
        for (const auto &property: properties) {
            if (!std::holds_alternative<std::monostate>(property.value)) continue;
            const auto node = create_node(property, parent);
            process_groups(property.children, node);
        }
    }

    ApexAppState &app_;
    VM::SceneBuilder &builder_;
    std::unordered_map<uint32, std::optional<uint32>> lod_targets_;
    std::unordered_map<uint32, std::shared_ptr<VM::Model>> models_;
};
}

VM::NodePtr Apex::export_pcbb(ApexAppState &app_state, uint64 path_hash, const std::unique_ptr<IO::File> &&buffer) {
    ZoneScoped
    const PCBB::PropertyFile file = PCBB::PropertyFile::from_file(*buffer);
    const auto path = find_name(path_hash).value_or(std::format("path_{:08X}", path_hash));
    const auto json_path = app_state.export_path() / (path + ".json");
    std::filesystem::create_directories(json_path.parent_path());
    std::ofstream output(json_path);
    if (!output) throw std::runtime_error("Failed to open PCBB JSON output: " + json_path.string());
    output << file.to_json().dump(2);
    if (!output) throw std::runtime_error("Failed to write PCBB JSON output: " + json_path.string());

    return PcbbSceneExporter(app_state).build(file);
}
