#include "exporter/apex/amf_material_merged.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <ranges>
#include <stdexcept>

#include "apex/hashes.h"
#include "exporter/ddsc_export.h"
#include "redscore/platform/logger.h"
#include "redscore/platform/texture/texture.h"
#include "redscore/utils/common.h"
#include "utils/hash_helper.h"

namespace {
    // BC5 X/Y normals are written as LA; reconstruct Z for glTF's RGB normal map.
    Texture::Ptr unpack_normal(Texture::Ptr texture) {
        const size_t channels = texture->channel_count();
        const size_t count = size_t(texture->width()) * texture->height();
        if (texture->bpc() != 1 || texture->is_float() || texture->depth() != 1 ||
            (channels != 2 && channels != 4) || texture->data().size() != count * channels)
            throw std::runtime_error("Unsupported Rage2 normal texture layout");

        const auto blue = [](uint8 red, uint8 green) -> uint8 {
            const float x = red / 127.5f - 1.f;
            const float y = green / 127.5f - 1.f;
            const float z = std::sqrt(std::max(0.f, 1.f - x * x - y * y));
            return static_cast<uint8>(std::lround((z * 0.5f + 0.5f) * 255.f));
        };
        if (channels == 2) {
            std::vector<uint8> rgba(count * 4);
            const auto &src = texture->data();
            for (size_t i = 0; i < count; ++i) {
                const uint8 red = src[i * 2], green = src[i * 2 + 1];
                rgba[i * 4] = red;
                rgba[i * 4 + 1] = green;
                rgba[i * 4 + 2] = blue(red, green);
                rgba[i * 4 + 3] = 255;
            }
            return std::make_unique<Texture>(texture->width(), texture->height(), 1, 1, 4, false,
                                             std::move(rgba));
        }
        auto &rgba = texture->data();
        for (size_t i = 0; i < count; ++i) {
            uint8 *pixel = rgba.data() + i * 4;
            pixel[1] = pixel[3];
            pixel[2] = blue(pixel[0], pixel[1]);
            pixel[3] = 255;
        }
        return texture;
    }
}

void export_amf_merged_material(ApexAppState &app_state,
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

    // Deferred attributes have no matching generated constants for these blocks.
    if ((render_block_id != "GeneralR2" && render_block_id != "Character") || app_state.skip_textures)
        return;
    const auto load_slot = [&](size_t slot) -> VM::TexturePtr {
        if (slot >= amf_material.Textures.size()) return {};
        const auto texture_name = find_lookup3_name(amf_material.Textures[slot].storage);
        if (!texture_name) return {};
        try {
            const auto hash = asset_path_hash(*texture_name);
            auto texture = convert_ddsc(app_state, hash);
            if (!texture) return {};
            if (slot == 1) texture = unpack_normal(texture);
            return VM::png_texture(
                std::format("{}_{:08X}.png", path_utils::filename(*texture_name), hash),
                texture->save_to_memory(MemoryFormat::PNG));
        } catch (const std::exception &error) {
            GLog_Warning("Unable to decode AMF texture {}: {}", *texture_name, error.what());
            return {};
        }
    };
    new_material->albedo = load_slot(0);
    new_material->normal = load_slot(1);
    new_material->metallic_roughness = load_slot(2);
}
