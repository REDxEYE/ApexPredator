#include "exporter/adf_export.h"

#include "apex/adf/adf_base_type.h"
#include "apex/hashes.h"
#include "redscore/platform/logger.h"
#include "redscore/utils/simple_fileio.h"
#include "tracy/Tracy.hpp"

#include <cstring>
#include <format>
#include <fstream>

extern ADF::TypeInfoMap adf_type_info;

VM::NodePtr export_adf_file(ApexAppState &app_state, const uint64 path_hash) {
    ZoneScoped
    auto result = app_state.manager().get(path_hash);
    if (!result) {
        GLog_Error("File not found\n");
        return {};
    }
    return export_adf_file_from_buffer(app_state, path_hash, std::move(result));
}

VM::NodePtr export_adf_file_from_buffer(ApexAppState &app_state, const uint64 path_hash,
                                        std::unique_ptr<IO::File> mb) {
    ZoneScoped
    uint32 small_type_alignment = 0;
    const auto &bytes = mb->cbuffer();
    if (bytes.size() >= sizeof(ADF::SmallHeader) &&
        std::memcmp(bytes.data(), ADF_SMALL_MAGIC, 4) == 0) {
        uint32 type_hash;
        std::memcpy(&type_hash, bytes.data() + 4, sizeof(type_hash));
        const auto type = adf_type_info.find(type_hash);
        if (type == adf_type_info.end() || !type->second->new_instance) {
            GLog_Error("Cannot decode small ADF: type 0x{:08X} has no generated binding", type_hash);
            return {};
        }
        small_type_alignment = type->second->alignment;
    }
    ADF::ADFFile adf = ADF::ADFFile::from_buffer(std::move(mb), small_type_alignment);
    const auto &instances = adf.instances();

    for (int instance_id = 0; instance_id < instances.size(); ++instance_id) {
        if (auto action = export_game_adf_instance(app_state, adf, instance_id, path_hash)) {
            if (action->stop) {
                return action->scene;
            }
            continue;
        }

        const auto &instance = instances[instance_id];
        const auto name = adf.get_string(instance.name_id);
        const auto data = adf.get_instance_data(instance_id);
        const auto &type_info = adf_type_info[instance.type_hash];
        GLog_Info("Dumping instance \"{}\" of type \"{}\" as json", name, type_info->name);

        auto path = find_asset_name(path_hash).value_or(std::format("unknown_{:08X}", path_hash));
        path += std::format("_{:08X}", instance.type_hash);
        std::filesystem::path dump_path = app_state.export_path() / path;
        std::filesystem::create_directories(dump_path.parent_path());
        write_file(dump_path, data);

        dump_path.replace_extension("json");
        std::ofstream json_out(dump_path);
        const auto instance_obj = adf.read_instance<ADF::BaseType>(instance_id);
        json_out << instance_obj->to_json().dump(2);
        json_out.close();
    }
    return {};
}
