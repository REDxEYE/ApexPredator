// Created by RED on 23.09.2025.

#define BCDEC_IMPLEMENTATION
#include "bcdec.h"

#include "nlohmann/json.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "redscore/stb_image.h"
#include "redscore/stb_image_write.h"

// #define TINYGLTF_NO_STB_IMAGE_WRITE
// #define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#include "redscore/gltf/tiny_gltf.h"