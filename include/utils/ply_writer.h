// Created by RED on 23.09.2025.

#ifndef APEXPREDATOR_PLY_WRITER_H
#define APEXPREDATOR_PLY_WRITER_H
#include <stdbool.h>
#include <stdint.h>
#include "utils/buffer/buffer.h"

/*
 Writes a binary little-endian PLY triangle mesh to the given Buffer.
 positions: [vertex_count*3] xyz
 normals:   optional [vertex_count*3] nx ny nz
 uvs:       optional [vertex_count*2] s t
 colors:    optional [vertex_count*color_stride], color_stride 3=RGB or 4=RGBA, bytes 0..255
 indices:   [index_count], multiple of 3; each face written as list uchar(3) + 3*int32
 Returns 0 on success, -1 on error with errno set.
*/
BufferError ply_write_tri_mesh_binary_buf(
    Buffer* buffer,
    const float* positions,
    const float* normals,
    const float* uvs,
    const uint8_t* colors,
    uint32_t color_stride,
    uint32_t vertex_count,
    const uint32* indices,
    uint32_t index_count
);
#endif //APEXPREDATOR_PLY_WRITER_H