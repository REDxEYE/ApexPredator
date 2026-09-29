//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the
// public domain. The author hereby disclaims copyright to this source
// code.

#pragma once

#include <stdint.h>

uint64_t MurmurHash3_x64_128(const void *key, uint32_t len, uint32_t seed);

