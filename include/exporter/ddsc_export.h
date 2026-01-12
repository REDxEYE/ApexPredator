// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_DDSC_EXPORT_H
#define APEXPREDATOR_DDSC_EXPORT_H
#include "apex/adf/sti.h"
#include "platform/archive_manager.h"

void export_ddsc(ArchiveManager *archive_manager, STI_TypeLibrary *lib, uint32 hash, MemoryBuffer *mb,
                 const String *path,
                 const String *export_path);


#endif //APEXPREDATOR_DDSC_EXPORT_H
