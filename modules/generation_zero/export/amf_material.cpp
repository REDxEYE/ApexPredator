#include "exporter/apex/amf_policy.h"

#include <format>
#include <ranges>

#include "apex/adf/adf.h"
#include "apex/hashes.h"
#include "exporter/ddsc_export.h"
#include "redscore/platform/logger.h"
#include "redscore/platform/texture/texture_ops.h"
#include "redscore/utils/common.h"
#include "utils/hash_helper.h"

void export_amf_material(ApexAppState &app_state, const VM::NodePtr &,
                         const ADFTypes::AmfMaterial &amf_material,
                         std::string_view material_name, std::string_view render_block_id) {
    auto &helper = app_state.models();
    if (helper.find_material(material_name)) return;

    auto new_material = helper.material(material_name);
    new_material->name = material_name;
    GLog_Info("Material {} -> {}", material_name, render_block_id);
    for (const auto [tex_id, texture]: amf_material.Textures | std::views::enumerate) {
        if (auto tex_path = find_name(texture.storage))
            GLog_Info("\tSlot {} -> {}", tex_id, *tex_path);
    }

    if (render_block_id != "GeneralR2") {
        GLog_Warning("Unsupported material render block: 0x{:08X}", amf_material.RenderBlockId.storage);
        return;
    }
    if (app_state.skip_textures) return;
    const auto constants = ADF::as<ADFTypes::GeneralR2Constants>(amf_material.Attributes);
    if (!constants) {
        GLog_Warning("Unsupported GeneralR2 material attribute type");
        return;
    }
    new_material->base_color[0] = 1.0f;
    new_material->base_color[1] = 1.0f;
    new_material->base_color[2] = 1.0f;
    new_material->base_color[3] = 1.0f;
    new_material->metallic_factor = 1.0f;
    new_material->roughness_factor = 1.0f;
    if (amf_material.Textures.size() < 3) return;

    Texture::Ptr diffuse_texture;
    if (const auto diffuse_path = find_name(amf_material.Textures[0])) {
        diffuse_texture = convert_ddsc(app_state, amf_material.Textures[0]);
        new_material->albedo = VM::png_texture(
            std::format("{}_{:08X}.png", path_utils::filename(*diffuse_path), asset_path_hash(*diffuse_path)),
            diffuse_texture->save_to_memory(MemoryFormat::PNG));
    }
    if (const auto normal_path = find_name(amf_material.Textures[1])) {
        auto normal_texture = convert_ddsc(app_state, amf_material.Textures[1]);
        new_material->normal = VM::png_texture(
            std::format("{}_{:08X}.png", path_utils::filename(*normal_path), asset_path_hash(*normal_path)),
            normal_texture->save_to_memory(MemoryFormat::PNG));
        new_material->normal_scale = 1.0f;
    }
    if (const auto orm_path = find_name(amf_material.Textures[2])) {
        auto orm_texture = convert_ddsc(app_state, amf_material.Textures[2]);
        new_material->metallic_roughness = VM::png_texture(
            std::format("{}_{:08X}.png", path_utils::filename(*orm_path), asset_path_hash(*orm_path)),
            orm_texture->save_to_memory(MemoryFormat::PNG));
    }
    if (constants->UseEmissive && amf_material.Textures.size() > 4) {
        if (const auto emission_path = find_name(amf_material.Textures[4])) {
            const uint64 hash = asset_path_hash(*emission_path);
            auto texture_name = std::format("{}_{:08X}", path_utils::filename(*emission_path), hash);
            auto emission_texture = convert_ddsc(app_state, amf_material.Textures[4]);
            if (!constants->EmissiveTextureHasColor && diffuse_texture) {
                auto new_emission_texture = TextureOps::multiply(emission_texture, diffuse_texture);
                auto texture_save_path = app_state.export_path();
                texture_save_path /= texture_name;
                if (new_emission_texture) {
                    new_emission_texture->save(texture_save_path);
                    emission_texture = std::move(new_emission_texture);
                }
            }
            new_material->emissive = VM::png_texture(texture_name,
                                                      emission_texture->save_to_memory(MemoryFormat::PNG));
            new_material->emissive_factor[0] = 1.0f;
            new_material->emissive_factor[1] = 1.0f;
            new_material->emissive_factor[2] = 1.0f;
        }
    }
}
