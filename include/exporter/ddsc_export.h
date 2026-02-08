// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_DDSC_EXPORT_H
#define APEXPREDATOR_DDSC_EXPORT_H
#include "apex/adf/sti.h"
#include "platform/app_state.h"
#include "platform/archive_manager.h"
#include "platform/texture.h"
#include "utils/string_view.h"

Texture* convert_ddsc(AppState* app_state, uint32 hash);

void export_ddsc(AppState* app_state, uint32 hash, MemoryBuffer *mb);


#endif //APEXPREDATOR_DDSC_EXPORT_H
