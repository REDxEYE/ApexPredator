// Created by RED on 01.02.2026.

#ifndef APEXPREDATOR_APP_STATE_H
#define APEXPREDATOR_APP_STATE_H
#include "archive_manager.h"
#include "utils/gltf/cgltf_helper.h"

typedef struct AppState {
    GLTFContext gltf_context;
    String game_root;
    String export_path;
    ArchiveManager archive_manager;

    bool export_textures;
}AppState;

void AppState_free(AppState* self);

#define CHECK_APP_STATE(state) \
    if (state == NULL) { \
        GLog_Error("AppState is NULL!"); \
        assert(false && "AppState is NULL!"); \
        abort(); \
    }

#endif //APEXPREDATOR_APP_STATE_H
