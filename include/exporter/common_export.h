// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_COMMON_EXPORT_H
#define APEXPREDATOR_COMMON_EXPORT_H
#include "apex/adf/sti.h"
#include "platform/app_state.h"
#include "platform/archive_manager.h"

GltfHelper::Handle<tinygltf::Node> export_file(ApexAppState& app_state, uint32 hash);

#endif //APEXPREDATOR_COMMON_EXPORT_H
