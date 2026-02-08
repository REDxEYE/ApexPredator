// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_EPE_EXPORT_H
#define APEXPREDATOR_EPE_EXPORT_H
#include "apex/rtpc.h"
#include "apex/adf/sti.h"
#include "havok/havok_codegen.h"
#include "platform/app_state.h"
#include "platform/archive_manager.h"
#include "utils/gltf/cgltf_helper.h"


void process_epe_node(AppState* app_state, RuntimeNode *node, uint32 path_hash, GL_ID parent_gltf_node);

GL_ID export_epe(AppState* app_state, RuntimeNode *root_node,uint32 path_hash);


#endif //APEXPREDATOR_EPE_EXPORT_H
