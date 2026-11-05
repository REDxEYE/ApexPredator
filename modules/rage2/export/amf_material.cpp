#include "exporter/apex/amf_material_merged.h"

#include <filesystem>
#include <string>

#include "apex/hashes.h"
#include "exporter/ddsc_export.h"
#include "redscore/platform/logger.h"
#include "redscore/platform/texture/texture.h"
#include "utils/hash_helper.h"

void export_amf_material(ApexAppState &app_state, const VM::NodePtr &model_root,
                         const ADFTypes::AmfMaterial &amf_material,
                         std::string_view material_name, std::string_view render_block_id) {
    export_amf_merged_material(app_state, amf_material, material_name, render_block_id);

    // Character slots 8/9 are auxiliary blood/gib maps, not the default PBR channels.
    if (render_block_id != "Character" || app_state.skip_textures) return;
    for (size_t slot: {8, 9}) {
        if (slot >= amf_material.Textures.size()) continue;
        const auto texture_name = find_lookup3_name(amf_material.Textures[slot].storage);
        if (!texture_name) continue;
        try {
            const auto hash = asset_path_hash(*texture_name);
            const auto source_path = std::filesystem::path(*texture_name);
            const auto destination = app_state.export_path() / source_path;
            auto image_path = source_path;
            image_path += ".png";
            if (!std::filesystem::exists(app_state.export_path() / image_path)) {
                auto texture = convert_ddsc(app_state, hash);
                if (!texture) continue;
                if (texture->is_float()) image_path.replace_extension(".dds");
                if (!texture->is_float() || !std::filesystem::exists(app_state.export_path() / image_path)) {
                    std::filesystem::create_directories(destination.parent_path());
                    texture->save(destination);
                }
            }
            model_root->extras["rage2CharacterTextures"][std::string(material_name)][std::to_string(slot)] =
                image_path.generic_string();
        } catch (const std::exception &error) {
            GLog_Warning("Unable to export Character texture {}: {}", *texture_name, error.what());
        }
    }
}
