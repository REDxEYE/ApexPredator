// Created by RED on 01.10.2025.

#include "utils/zlib_wrapper.h"

#include "memory.h"

int inflate_exact_into(const void* in_buf, size_t in_len,
                       void* out_buf, size_t expected_out_len,
                       int windowBits,
                       size_t* out_written, size_t* in_consumed)
{
    z_stream s;
    memset(&s, 0, sizeof(s));

    int rc = inflateInit2(&s, windowBits);
    if (rc != Z_OK) return rc;

    s.next_in = (Bytef*)in_buf;
    s.avail_in = 0;

    size_t produced_before = 0;

    for (;;) {
        if (s.avail_in == 0 && (size_t)(s.next_in - (Bytef*)in_buf) < in_len) {
            size_t used = (size_t)(s.next_in - (Bytef*)in_buf);
            size_t chunk = in_len - used;
            if (chunk > (size_t)UINT_MAX) chunk = (size_t)UINT_MAX;
            s.next_in = (Bytef*)in_buf + used;
            s.avail_in = (uInt)chunk;
        }

        size_t remaining_out = expected_out_len - (size_t)s.total_out;
        if (remaining_out > (size_t)UINT_MAX) remaining_out = (size_t)UINT_MAX;
        s.next_out  = (Bytef*)out_buf + s.total_out;
        s.avail_out = (uInt)remaining_out;

        rc = inflate(&s, Z_NO_FLUSH);

        if (rc == Z_STREAM_END) {
            int ok = (size_t)s.total_out == expected_out_len;
            if (out_written)  *out_written  = (size_t)s.total_out;
            if (in_consumed)  *in_consumed  = (size_t)(s.next_in - (Bytef*)in_buf);
            inflateEnd(&s);
            return ok ? Z_OK : Z_DATA_ERROR;
        }

        if (rc != Z_OK) {
            if (out_written) *out_written = (size_t)s.total_out;
            if (in_consumed) *in_consumed = (size_t)(s.next_in - (Bytef*)in_buf);
            inflateEnd(&s);
            return rc;
        }

        if (s.total_out == produced_before && s.avail_in == 0 && s.avail_out != 0) {
            if (out_written) *out_written = (size_t)s.total_out;
            if (in_consumed) *in_consumed = (size_t)(s.next_in - (Bytef*)in_buf);
            inflateEnd(&s);
            return Z_BUF_ERROR;
        }

        produced_before = s.total_out;

        if ((size_t)s.total_out > expected_out_len) {
            if (out_written) *out_written = (size_t)s.total_out;
            if (in_consumed) *in_consumed = (size_t)(s.next_in - (Bytef*)in_buf);
            inflateEnd(&s);
            return Z_BUF_ERROR;
        }
    }
}