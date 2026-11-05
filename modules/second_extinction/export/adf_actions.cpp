#include "exporter/adf_export.h"
#include "apex/adf/generated/adf_types.h"
#include "apex/hashes.h"
#include "exporter/amf_export.h"
#include "exporter/apex/adf_dialogue.hpp"

using namespace ADFTypes;

std::optional<AdfAction> export_game_adf_instance(ApexAppState &app_state, ADF::ADFFile &adf,
                                                   int &instance_id, const uint64 path_hash) {
    const auto &instances = adf.instances();
    const auto type_hash = instances[instance_id].type_hash;
    if (type_hash == std::to_underlying(ADFHashes::AmfModelVariant)) {
        if (instances.size() != 1) {
            throw std::runtime_error("ADF with AmfModelVariant should have only one instance");
        }
        const auto variant = adf.read_instance<AmfModelVariant>(instance_id);
        return AdfAction{true, export_amf_model(app_state, &variant->Data, path_hash)};
    }
    if (type_hash == std::to_underlying(ADFHashes::StringLookup)) {
        if (instances.size() != 1) {
            throw std::runtime_error("ADF with StringLookup should have only one instance");
        }
        const auto lookup = adf.read_instance<StringLookup>(instance_id);
        export_apex_dialogue<false>(app_state, *lookup, path_hash, type_hash);
        return AdfAction{};
    }
    return export_apex_adf_instance(app_state, adf, instance_id, path_hash);
}
