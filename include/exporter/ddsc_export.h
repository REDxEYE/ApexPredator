// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_DDSC_EXPORT_H
#define APEXPREDATOR_DDSC_EXPORT_H
#include "apex/adf/sti.h"
#include "platform/app_state.h"
#include "platform/texture.h"

std::unique_ptr<Texture> convert_ddsc(AppState& app_state, uint32 hash);

void export_ddsc(AppState& app_state, uint32 hash, std::unique_ptr<IO::File> &&mb);


#endif //APEXPREDATOR_DDSC_EXPORT_H
