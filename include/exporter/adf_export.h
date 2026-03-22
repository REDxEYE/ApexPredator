// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_ADF_EXPORT_H
#define APEXPREDATOR_ADF_EXPORT_H
#include "apex/adf/sti.h"
#include "platform/app_state.h"

GltfHelper::Handle<tinygltf::Node> export_adf_file(ApexAppState &app_state, uint32 path_hash);

GltfHelper::Handle<tinygltf::Node> export_adf_file_from_buffer(ApexAppState &app_state, uint32 path_hash, std::unique_ptr<IO::File> mb);


#endif //APEXPREDATOR_ADF_EXPORT_H
