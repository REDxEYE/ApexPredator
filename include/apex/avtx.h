// Created by RED on 30.09.2025.

#ifndef APEXPREDATOR_AVTX_H
#define APEXPREDATOR_AVTX_H
#include "int_def.h"
#include "platform/texture.h"
#include "utils/buffer/buffer.h"

#define AVTX_MAGIC "AVTX"

typedef struct {
    char ident[4];
    uint16 version;
    uint8 unk0;
    uint8 resource_dimensions;
    DDSDXGIFormat format;
    uint16 width, height, depth;
    uint16 flags;
    uint8 mip_count;
    uint16 unk1;
    uint32 unk2,unk3;
    uint32 header_size;
    uint32 body_size;
}AVTXHeader;

void AVTXTexture_from_buffer(Buffer* buffer, Texture* texture);

#endif //APEXPREDATOR_AVTX_H