// Created by RED on 01.02.2026.

#include "platform/app_state.h"

void AppState_free(AppState *self) {
    ArchiveManager_free(&self->archive_manager);
    String_free(&self->game_root);
    String_free(&self->export_path);
    if (self->gltf_context.data!=NULL) {
        GLTFContext_free(&self->gltf_context);
    }
}
