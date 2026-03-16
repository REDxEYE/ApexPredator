// Created by RED on 11.03.2026.
#pragma once

#include <cstddef>
#include <cstdint>

#include "utils/memory_debugger.h"

#define mp_malloc(n) memory_debug_alloc((n), __FILE__, static_cast<std::uint32_t>(__LINE__), __func__)
#define mp_calloc(count, size) memory_debug_calloc((count), (size), __FILE__, static_cast<std::uint32_t>(__LINE__), __func__)
#define mp_realloc(ptr, size) memory_debug_realloc((ptr), (size), __FILE__, static_cast<std::uint32_t>(__LINE__), __func__)
#define mp_free(ptr) memory_debug_free((ptr), __FILE__, static_cast<std::uint32_t>(__LINE__), __func__)

