// Created by RED on 11.03.2026.
#include "utils/memory_debugger.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#if defined(_MSC_VER)
#include <malloc.h>
#endif
#include <new>

#include "tracy/Tracy.hpp"
#include "tracy/TracyC.h"

namespace {
    struct AllocationHeader {
        std::uint64_t magic;
        std::size_t size;
        const char *file;
        const char *function;
        std::uint32_t line;
        void *user_ptr;
        AllocationHeader *prev;
        AllocationHeader *next;
    };

    constexpr std::uint64_t kAllocMagic = 0x415045584D454D30ULL; // "APEXMEM0"
    constexpr std::uint64_t kFreedMagic = 0x46524545444D454DULL; // "FREEDMEM"

    std::atomic_flag g_lock = ATOMIC_FLAG_INIT;
    AllocationHeader *g_head = nullptr;

    std::atomic<std::size_t> g_active_allocations{0};
    std::atomic<std::size_t> g_active_bytes{0};
    std::atomic<std::size_t> g_total_allocations{0};
    std::atomic<std::size_t> g_total_frees{0};
    std::atomic<std::size_t> g_peak_bytes{0};

    std::atomic<bool> g_initialized{false};
    std::atomic<bool> g_shutdown{false};
    std::atomic<bool> g_registered_atexit{false};

    class SpinGuard {
    public:
        SpinGuard() {
            while (g_lock.test_and_set(std::memory_order_acquire)) {
            }
        }

        ~SpinGuard() {
            g_lock.clear(std::memory_order_release);
        }
    };

    void update_peak_bytes(const std::size_t active_bytes) {
        std::size_t current_peak = g_peak_bytes.load(std::memory_order_relaxed);
        while (active_bytes > current_peak && !g_peak_bytes.compare_exchange_weak(
                   current_peak, active_bytes, std::memory_order_relaxed, std::memory_order_relaxed)) {
        }
    }

    void list_insert(AllocationHeader *header) {
        header->prev = nullptr;
        header->next = g_head;
        if (g_head != nullptr) {
            g_head->prev = header;
        }
        g_head = header;
    }

    void list_remove(AllocationHeader *header) {
        if (header->prev != nullptr) {
            header->prev->next = header->next;
        }
        else {
            g_head = header->next;
        }

        if (header->next != nullptr) {
            header->next->prev = header->prev;
        }

        header->prev = nullptr;
        header->next = nullptr;
    }

    [[nodiscard]] std::size_t safe_multiply(std::size_t lhs, std::size_t rhs) {
        if (lhs == 0 || rhs == 0) {
            return 0;
        }
        if (lhs > (static_cast<std::size_t>(-1) / rhs)) {
            return 0;
        }
        return lhs * rhs;
    }

    void memory_debug_atexit() {
        memory_debug_shutdown();
    }

    inline void tracy_alloc_event(void *ptr, const std::size_t size) {
#if defined(TRACY_ENABLE)
        TracyAlloc(ptr, size);
#else
        (void) ptr;
        (void) size;
#endif
    }

    inline void tracy_free_event(void *ptr) {
#if defined(TRACY_ENABLE)
        TracyFree(ptr);
#else
        (void) ptr;
#endif
    }

    void *alloc_aligned_tracked(const std::size_t size, const std::size_t align, const char *file,
                                const std::uint32_t line, const char *function) {
        if (align == 0 || (align & (align - 1)) != 0) {
            return nullptr;
        }

        const std::size_t prefix = sizeof(void *);
        const std::size_t padding = align - 1;
        constexpr std::size_t header_size = sizeof(AllocationHeader);
        if (size > static_cast<std::size_t>(-1) - header_size - prefix - padding) {
            return nullptr;
        }

        auto *raw_header = static_cast<AllocationHeader *>(std::malloc(header_size + prefix + padding + size));
        if (raw_header == nullptr) {
            return nullptr;
        }

        const std::uintptr_t start = reinterpret_cast<std::uintptr_t>(raw_header + 1) + prefix;
        const std::uintptr_t aligned_addr = (start + (align - 1)) & ~(static_cast<std::uintptr_t>(align) - 1);
        auto *user_ptr = reinterpret_cast<void *>(aligned_addr);
        reinterpret_cast<void **>(user_ptr)[-1] = raw_header;

        raw_header->magic = kAllocMagic;
        raw_header->size = size;
        raw_header->file = file;
        raw_header->function = function;
        raw_header->line = line;
        raw_header->user_ptr = user_ptr;
        raw_header->prev = nullptr;
        raw_header->next = nullptr;

        {
            const SpinGuard guard;
            list_insert(raw_header);
        }

        g_total_allocations.fetch_add(1, std::memory_order_relaxed);
        g_active_allocations.fetch_add(1, std::memory_order_relaxed);
        const std::size_t active = g_active_bytes.fetch_add(size, std::memory_order_relaxed) + size;
        update_peak_bytes(active);
        tracy_alloc_event(user_ptr, size);

        return user_ptr;
    }

    void free_aligned_tracked(void *ptr) {
        if (ptr == nullptr) {
            return;
        }

        auto *header = static_cast<AllocationHeader *>(reinterpret_cast<void **>(ptr)[-1]);
        if (header == nullptr || header->magic != kAllocMagic) {
            std::fprintf(stderr, "memory_debug: invalid aligned free at %p\n", ptr);
            return;
        }

        {
            const SpinGuard guard;
            list_remove(header);
        }

        header->magic = kFreedMagic;
        g_total_frees.fetch_add(1, std::memory_order_relaxed);
        g_active_allocations.fetch_sub(1, std::memory_order_relaxed);
        g_active_bytes.fetch_sub(header->size, std::memory_order_relaxed);
        tracy_free_event(header->user_ptr);
        std::free(header);
    }
}

void memory_debug_init() {
    const bool already_initialized = g_initialized.exchange(true, std::memory_order_acq_rel);
    if (already_initialized) {
        return;
    }

    if (!g_registered_atexit.exchange(true, std::memory_order_acq_rel)) {
        std::atexit(memory_debug_atexit);
    }

    g_shutdown.store(false, std::memory_order_release);
}

void memory_debug_shutdown() {
    if (!g_initialized.load(std::memory_order_acquire)) {
        return;
    }

    if (g_shutdown.exchange(true, std::memory_order_acq_rel)) {
        return;
    }

    memory_debug_dump_leaks();
}

void *memory_debug_alloc(const std::size_t size, const char *file, const std::uint32_t line, const char *function) {
    memory_debug_init();

    constexpr std::size_t header_size = sizeof(AllocationHeader);
    if (size > static_cast<std::size_t>(-1) - header_size) {
        return nullptr;
    }

    auto *raw = static_cast<AllocationHeader *>(std::malloc(header_size + size));
    if (raw == nullptr) {
        return nullptr;
    }

    raw->magic = kAllocMagic;
    raw->size = size;
    raw->file = file;
    raw->function = function;
    raw->line = line;
    raw->user_ptr = raw + 1;
    raw->prev = nullptr;
    raw->next = nullptr;

    {
        const SpinGuard guard;
        list_insert(raw);
    }

    g_total_allocations.fetch_add(1, std::memory_order_relaxed);
    g_active_allocations.fetch_add(1, std::memory_order_relaxed);
    const std::size_t active = g_active_bytes.fetch_add(size, std::memory_order_relaxed) + size;
    update_peak_bytes(active);
    tracy_alloc_event(raw->user_ptr, size);

    return raw->user_ptr;
}

void *memory_debug_calloc(const std::size_t count, const std::size_t size, const char *file, const std::uint32_t line,
                          const char *function) {
    const std::size_t total = safe_multiply(count, size);
    if (count != 0 && size != 0 && total == 0) {
        return nullptr;
    }

    void *ptr = memory_debug_alloc(total, file, line, function);
    if (ptr != nullptr && total > 0) {
        std::memset(ptr, 0, total);
    }
    return ptr;
}

void *memory_debug_realloc(void *ptr, const std::size_t size, const char *file, const std::uint32_t line,
                           const char *function) {
    if (ptr == nullptr) {
        return memory_debug_alloc(size, file, line, function);
    }

    if (size == 0) {
        memory_debug_free(ptr, file, line, function);
        return nullptr;
    }

    auto *old_header = static_cast<AllocationHeader *>(ptr) - 1;
    if (old_header->magic != kAllocMagic) {
        return nullptr;
    }

    constexpr std::size_t header_size = sizeof(AllocationHeader);
    if (size > static_cast<std::size_t>(-1) - header_size) {
        return nullptr;
    }

    std::size_t old_size = 0;
    void *old_user_ptr = nullptr;
    {
        const SpinGuard guard;
        old_size = old_header->size;
        old_user_ptr = old_header->user_ptr;
        list_remove(old_header);
    }

    auto *new_header = static_cast<AllocationHeader *>(std::realloc(old_header, header_size + size));
    if (new_header == nullptr) {
        {
            const SpinGuard guard;
            list_insert(old_header);
        }
        return nullptr;
    }

    new_header->magic = kAllocMagic;
    new_header->size = size;
    new_header->file = file;
    new_header->function = function;
    new_header->line = line;
    new_header->user_ptr = new_header + 1;

    {
        const SpinGuard guard;
        list_insert(new_header);
    }

    if (size >= old_size) {
        const std::size_t delta = size - old_size;
        const std::size_t active = g_active_bytes.fetch_add(delta, std::memory_order_relaxed) + delta;
        update_peak_bytes(active);
    }
    else {
        g_active_bytes.fetch_sub(old_size - size, std::memory_order_relaxed);
    }

    tracy_free_event(old_user_ptr);
    tracy_alloc_event(new_header->user_ptr, size);
    return new_header->user_ptr;
}

void memory_debug_free(void *ptr, const char *, const std::uint32_t, const char *) {
    if (ptr == nullptr) {
        return;
    }

    auto *header = static_cast<AllocationHeader *>(ptr) - 1;
    if (header->magic != kAllocMagic) {
        std::fprintf(stderr, "memory_debug: invalid or double free at %p\n", ptr);
        return;
    }

    {
        const SpinGuard guard;
        list_remove(header);
    }

    header->magic = kFreedMagic;

    g_total_frees.fetch_add(1, std::memory_order_relaxed);
    g_active_allocations.fetch_sub(1, std::memory_order_relaxed);
    g_active_bytes.fetch_sub(header->size, std::memory_order_relaxed);
    tracy_free_event(header->user_ptr);

    std::free(header);
}

MemoryDebugStats memory_debug_stats() {
    MemoryDebugStats stats{};
    stats.active_allocations = g_active_allocations.load(std::memory_order_relaxed);
    stats.active_bytes = g_active_bytes.load(std::memory_order_relaxed);
    stats.total_allocations = g_total_allocations.load(std::memory_order_relaxed);
    stats.total_frees = g_total_frees.load(std::memory_order_relaxed);
    stats.peak_bytes = g_peak_bytes.load(std::memory_order_relaxed);
    return stats;
}

void memory_debug_dump_leaks() {
    const SpinGuard guard;
    std::size_t leaks = 0;
    std::size_t leak_bytes = 0;

    for (auto *cur = g_head; cur != nullptr; cur = cur->next) {
        ++leaks;
        leak_bytes += cur->size;
        const char *file = cur->file ? cur->file : "<unknown>";
        const char *func = cur->function ? cur->function : "<unknown>";
        // std::fprintf(stderr, "LEAK ptr=%p size=%zu at %s:%u (%s)\n", cur->user_ptr, cur->size, file, cur->line, func);
    }

    const MemoryDebugStats stats = memory_debug_stats();
    std::fprintf(stderr,
                 "MemoryDebug summary: active_allocs=%zu active_bytes=%zu total_allocs=%zu total_frees=%zu peak_bytes=%zu leaks=%zu leak_bytes=%zu\n",
                 stats.active_allocations,
                 stats.active_bytes,
                 stats.total_allocations,
                 stats.total_frees,
                 stats.peak_bytes,
                 leaks,
                 leak_bytes);
}

void *operator new(std::size_t size) {
    if (void *p = memory_debug_alloc(size, "<new>", 0, "operator new")) {
        return p;
    }
    throw std::bad_alloc();
}

void *operator new[](std::size_t size) {
    if (void *p = memory_debug_alloc(size, "<new[]>", 0, "operator new[]")) {
        return p;
    }
    throw std::bad_alloc();
}

void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
    return memory_debug_alloc(size, "<new nothrow>", 0, "operator new");
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
    return memory_debug_alloc(size, "<new[] nothrow>", 0, "operator new[]");
}

void operator delete(void *ptr) noexcept {
    memory_debug_free(ptr, "<delete>", 0, "operator delete");
}

void operator delete[](void *ptr) noexcept {
    memory_debug_free(ptr, "<delete[]>", 0, "operator delete[]");
}

void operator delete(void *ptr, std::size_t) noexcept {
    memory_debug_free(ptr, "<delete sized>", 0, "operator delete");
}

void operator delete[](void *ptr, std::size_t) noexcept {
    memory_debug_free(ptr, "<delete[] sized>", 0, "operator delete[]");
}

void operator delete(void *ptr, const std::nothrow_t &) noexcept {
    memory_debug_free(ptr, "<delete nothrow>", 0, "operator delete");
}

void operator delete[](void *ptr, const std::nothrow_t &) noexcept {
    memory_debug_free(ptr, "<delete[] nothrow>", 0, "operator delete[]");
}

void *operator new(std::size_t size, std::align_val_t alignment) {
    const std::size_t align = static_cast<std::size_t>(alignment);
    if (void *p = alloc_aligned_tracked(size, std::max(align, alignof(std::max_align_t)), "<new aligned>", 0,
                                        "operator new")) {
        return p;
    }
    if (void *p = alloc_aligned_tracked(1, std::max(align, alignof(std::max_align_t)), "<new aligned>", 0,
                                        "operator new")) {
        return p;
    }
    throw std::bad_alloc();
}

void *operator new[](std::size_t size, std::align_val_t alignment) {
    return operator new(size, alignment);
}

void *operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t &) noexcept {
    const std::size_t align = static_cast<std::size_t>(alignment);
    if (void *p = alloc_aligned_tracked(size, std::max(align, alignof(std::max_align_t)),
                                        "<new aligned nothrow>", 0, "operator new")) {
        return p;
    }
    return alloc_aligned_tracked(1, std::max(align, alignof(std::max_align_t)), "<new aligned nothrow>", 0,
                                 "operator new");
}

void *operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t &) noexcept {
    return operator new(size, alignment, std::nothrow);
}

void operator delete(void *ptr, std::align_val_t alignment) noexcept {
    (void) alignment;
    free_aligned_tracked(ptr);
}

void operator delete[](void *ptr, std::align_val_t alignment) noexcept {
    operator delete(ptr, alignment);
}

void operator delete(void *ptr, std::align_val_t alignment, const std::nothrow_t &) noexcept {
    operator delete(ptr, alignment);
}

void operator delete[](void *ptr, std::align_val_t alignment, const std::nothrow_t &) noexcept {
    operator delete(ptr, alignment);
}

void operator delete(void *ptr, std::size_t, std::align_val_t alignment) noexcept {
    operator delete(ptr, alignment);
}

void operator delete[](void *ptr, std::size_t, std::align_val_t alignment) noexcept {
    operator delete(ptr, alignment);
}
