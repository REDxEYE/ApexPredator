// Created by RED on 01.10.2025.

#include "utils/zlib_wrapper.h"

#include <memory>
#include <climits>
#include <cstring>

int inflate_exact_into(const void* in_buf, size_t in_len,
                       void* out_buf, size_t expected_out_len,
                       int windowBits,
                       size_t* out_written, size_t* in_consumed)
{
    zng_stream s{};
    int rc = zng_inflateInit2(&s, windowBits);
    if (rc != Z_OK)
        return rc;

    if (in_len > UINT_MAX || expected_out_len > UINT_MAX) {
        zng_inflateEnd(&s);
        return Z_BUF_ERROR;
    }

    s.next_in = static_cast<Bytef*>(const_cast<void*>(in_buf));
    s.avail_in = static_cast<uInt>(in_len);
    s.next_out = static_cast<Bytef*>(out_buf);
    s.avail_out = static_cast<uInt>(expected_out_len);

    rc = zng_inflate(&s, Z_FINISH);

    if (out_written) *out_written = s.total_out;
    if (in_consumed) *in_consumed = static_cast<size_t>(s.next_in - static_cast<const Bytef*>(in_buf));

    zng_inflateEnd(&s);

    if (rc != Z_STREAM_END)
        return rc;

    if (s.total_out != expected_out_len)
        return Z_DATA_ERROR;

    return Z_OK;
}