// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_AMF_EXPORT_H
#define APEXPREDATOR_AMF_EXPORT_H
#include "apex/adf/sti.h"
#include "apex/adf/adf_types.h"
#include "havok/havok_codegen.h"
#include "platform/archive_manager.h"
#include "utils/gltf/cgltf_helper.h"

GL_ID export_amf_mesh(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib, String *export_path,
                      uint32 path_hash, const String *path, const AmfMeshHeader *header, const AmfMeshBuffers *mesh_buffers);
GL_ID export_amf_model(GLTFContext *context, ArchiveManager *archive_manager, STI_TypeLibrary *lib,
                       const AmfModel *amf_model,
                       const String *path, const uint32 path_hash, const String *export_path);

#endif //APEXPREDATOR_AMF_EXPORT_H