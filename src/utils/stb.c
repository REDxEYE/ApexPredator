// Created by RED on 23.09.2025.

#include "platform/memory_profiling.h"
#define STBIW_MALLOC(sz)        mp_malloc(sz)
#define STBIW_REALLOC(p,newsz)  mp_realloc(p,newsz)
#define STBIW_FREE(p)           mp_free(p)

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "utils/stb_image_write.h"

#define CGLTF_WRITE_IMPLEMENTATION
#include "cgltf_write.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#define BCDEC_IMPLEMENTATION
#include "bcdec.h"