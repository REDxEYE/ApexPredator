// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_EPE_EXPORT_H
#define APEXPREDATOR_EPE_EXPORT_H
#include "apex/rtpc.h"
#include "apex/adf/sti.h"
#include "havok/havok_codegen.h"
#include "platform/archive_manager.h"
#include "utils/gltf/cgltf_helper.h"


void process_epe_node(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                      Havok_TypeLibrary *havok_lib, RuntimeNode *node,
                      uint32 path_hash,
                      const String *path, const String *export_path, GL_ID parent_gltf_node);

GL_ID export_epe(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib, Havok_TypeLibrary *havok_lib,
                 RuntimeNode *root_node,
                 uint32 path_hash,
                 const String *path, const String *export_path);


#endif //APEXPREDATOR_EPE_EXPORT_H
