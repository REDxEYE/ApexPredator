// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_COMMON_EXPORT_H
#define APEXPREDATOR_COMMON_EXPORT_H
#include "apex/adf/sti.h"
#include "platform/archive_manager.h"
#include "utils/gltf/cgltf_helper.h"

void mount_archive(const ArchiveManager *manager, uint32 hash);

GL_ID export_file(GLTFContext *context, ArchiveManager *archive_manager,
                  STI_TypeLibrary *lib,
                  const String *path, uint32 hash, const String *export_path);

#endif //APEXPREDATOR_COMMON_EXPORT_H
