// Created by RED on 14.01.2026.

#ifndef APEXPREDATOR_TEXTURE_OPS_H
#define APEXPREDATOR_TEXTURE_OPS_H
#include <memory>
#include "texture.h"

namespace TextureOps {
    std::unique_ptr<Texture> multiply(const Texture *texture_a, const Texture *texture_b);
};


#endif //APEXPREDATOR_TEXTURE_OPS_H
