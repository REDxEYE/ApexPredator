// Created by RED on 01.10.2025.

#ifndef APEXPREDATOR_ZLIB_RAW_WRAPPER_H
#define APEXPREDATOR_ZLIB_RAW_WRAPPER_H

#include "zlib.h"
#include "int_def.h"

#define WB_RAW  (-MAX_WBITS)

int inflate_exact_into(const void* in_buf, size_t in_len,
                       void* out_buf, size_t expected_out_len,
                       int windowBits,
                       size_t* out_written, size_t* in_consumed);

static inline int inflate_exact_raw (const void* in_buf, size_t in_len, void* out_buf, size_t out_len, size_t* out_written, size_t* in_consumed) {
    return inflate_exact_into(in_buf, in_len, out_buf, out_len, WB_RAW,  out_written, in_consumed);
}

#endif //APEXPREDATOR_ZLIB_RAW_WRAPPER_H