// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_ADF_EXPORT_H
#define APEXPREDATOR_ADF_EXPORT_H
#include "apex/adf/sti.h"
#include "platform/app_state.h"
#include "platform/archive_manager.h"
#include "utils/gltf/cgltf_helper.h"

GL_ID export_adf_file(AppState *app_state, const String *path, uint32 path_hash);

GL_ID export_adf_file_from_buffer(AppState *app_state, uint32 path_hash, const String *path, MemoryBuffer *mb);


#endif //APEXPREDATOR_ADF_EXPORT_H
