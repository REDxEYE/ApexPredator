// Created by RED on 30.09.2025.

#include "../../include/apex/avtx.h"

#include "platform/logger.h"

void AVTXTexture_from_buffer(Buffer *buffer, Texture *texture) {
    AVTXHeader header;
    buffer->read(buffer, &header, sizeof(header), NULL);
    if (strncmp(header.ident, "AVTX", 4) != 0) {
        GLog_Error("Invalid AVTX texture format");
        exit(1);
    }
    if (header.version != 1) {
        GLog_Error("Unsupported AVTX version: %d", header.version);
        exit(1);
    }

}
