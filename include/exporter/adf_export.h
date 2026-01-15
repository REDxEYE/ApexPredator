// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_ADF_EXPORT_H
#define APEXPREDATOR_ADF_EXPORT_H
#include "apex/adf/sti.h"
#include "havok/havok_codegen.h"
#include "platform/archive_manager.h"
#include "utils/gltf/cgltf_helper.h"

GL_ID export_adf_file(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                              Havok_TypeLibrary *havok_lib, const String *path, uint32 path_hash, const String *export_path);

GL_ID export_adf_file_from_buffer(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                      Havok_TypeLibrary *havok_lib, uint32 path_hash, const String *path, MemoryBuffer *mb,
                      const String *export_path);


#endif //APEXPREDATOR_ADF_EXPORT_H
