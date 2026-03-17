// Created by RED on 01.10.2025.

#include "utils/zlib_wrapper.h"

#include <memory>
#include <climits>
#include <cstring>

int inflate_exact_into(const void* in_buf, size_t in_len,
                       void* out_buf, size_t expected_out_len,
                       const int windowBits,
                       size_t* out_written, size_t* in_consumed)
{
    zng_stream s = {};

    int rc = zng_inflateInit2(&s, windowBits);
    if (rc != Z_OK) return rc;

    s.next_in = static_cast<Bytef*>(const_cast<void*>(in_buf));
    s.avail_in = 0;

    size_t produced_before = 0;

    for (;;) {
        if (s.avail_in == 0 && static_cast<size_t>(s.next_in - static_cast<const Bytef*>(in_buf)) < in_len) {
            const auto used = static_cast<size_t>(s.next_in - static_cast<const Bytef*>(in_buf));
            size_t chunk = in_len - used;
            if (chunk > static_cast<size_t>(UINT_MAX)) chunk = static_cast<size_t>(UINT_MAX);
            s.next_in = static_cast<Bytef*>(const_cast<void*>(in_buf)) + used;
            s.avail_in = static_cast<uInt>(chunk);
        }

        size_t remaining_out = expected_out_len - s.total_out;
        if (remaining_out > static_cast<size_t>(UINT_MAX)) remaining_out = static_cast<size_t>(UINT_MAX);
        s.next_out  = static_cast<Bytef*>(out_buf) + s.total_out;
        s.avail_out = static_cast<uInt>(remaining_out);

        rc = zng_inflate(&s, Z_NO_FLUSH);

        if (rc == Z_STREAM_END) {
            const bool ok = s.total_out == expected_out_len;
            if (out_written)  *out_written  = s.total_out;
            if (in_consumed)  *in_consumed  = static_cast<size_t>(s.next_in - static_cast<const Bytef*>(in_buf));
            zng_inflateEnd(&s);
            return ok ? Z_OK : Z_DATA_ERROR;
        }

        if (rc != Z_OK) {
            if (out_written) *out_written = s.total_out;
            if (in_consumed) *in_consumed = static_cast<size_t>(s.next_in - static_cast<const Bytef*>(in_buf));
            zng_inflateEnd(&s);
            return rc;
        }

        if (s.total_out == produced_before && s.avail_in == 0 && s.avail_out != 0) {
            if (out_written) *out_written = s.total_out;
            if (in_consumed) *in_consumed = static_cast<size_t>(s.next_in - static_cast<const Bytef*>(in_buf));
            zng_inflateEnd(&s);
            return Z_BUF_ERROR;
        }

        produced_before = s.total_out;

        if (s.total_out > expected_out_len) {
            if (out_written) *out_written = s.total_out;
            if (in_consumed) *in_consumed = static_cast<size_t>(s.next_in - static_cast<const Bytef*>(in_buf));
            zng_inflateEnd(&s);
            return Z_BUF_ERROR;
        }
    }
}