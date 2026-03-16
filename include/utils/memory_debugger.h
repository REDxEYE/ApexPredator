// Created by RED on 11.03.2026.
#pragma once

#include <cstddef>
#include <cstdint>

struct MemoryDebugStats {
    std::size_t active_allocations = 0;
    std::size_t active_bytes = 0;
    std::size_t total_allocations = 0;
    std::size_t total_frees = 0;
    std::size_t peak_bytes = 0;
};

void memory_debug_init();
void memory_debug_shutdown();

void *memory_debug_alloc(std::size_t size, const char *file, std::uint32_t line, const char *function);
void *memory_debug_calloc(std::size_t count, std::size_t size, const char *file, std::uint32_t line, const char *function);
void *memory_debug_realloc(void *ptr, std::size_t size, const char *file, std::uint32_t line, const char *function);
void memory_debug_free(void *ptr, const char *file, std::uint32_t line, const char *function);

MemoryDebugStats memory_debug_stats();
void memory_debug_dump_leaks();

