// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_ADF_EXPORT_H
#define APEXPREDATOR_ADF_EXPORT_H
#include "apex/adf/adf.h"
#include "platform/app_state.h"
#include <optional>

VM::NodePtr export_adf_file(ApexAppState &app_state, uint64 path_hash);

VM::NodePtr export_adf_file_from_buffer(ApexAppState &app_state, uint64 path_hash, std::unique_ptr<IO::File> mb);
// A missing action falls through to the ordinary raw/JSON dump. A handled
// action may emit a file without a scene, or stop traversal with a scene.
struct AdfAction {
    bool stop = false;
    VM::NodePtr scene{};
};

// Exactly one game adapter supplies this hook in each Apex module.
std::optional<AdfAction> export_game_adf_instance(ApexAppState &app_state, ADF::ADFFile &adf,
                                                   int &instance_id, uint64 path_hash);

// Shared Apex format-family actions; only game adapters call this.
std::optional<AdfAction> export_apex_adf_instance(ApexAppState &app_state, ADF::ADFFile &adf,
                                                   int &instance_id, uint64 path_hash);


#endif //APEXPREDATOR_ADF_EXPORT_H
