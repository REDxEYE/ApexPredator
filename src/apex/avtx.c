// Created by RED on 30.09.2025.

#include "../../include/apex/avtx.h"

void AVTXTexture_from_buffer(Buffer *buffer, Texture *texture) {
    AVTXHeader header;
    buffer->read(buffer, &header, sizeof(header), NULL);
    if (strncmp(header.ident, "AVTX", 4) != 0) {
        printf("[ERROR]: Invalid AVTX texture format\n");
        exit(1);
    }
    if (header.version != 1) {
        printf("[ERROR]: Unsupported AVTX version: %d\n", header.version);
        exit(1);
    }

}
