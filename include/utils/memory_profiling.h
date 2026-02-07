// Created by RED on 18.01.2026.

#ifndef APEXPREDATOR_MEMORY_PROFILING_H
#define APEXPREDATOR_MEMORY_PROFILING_H
#include "tracy/TracyC.h"

#include "malloc.h"

void* tracy_xmalloc(size_t n, const char* file, uint32_t line, const char* func);

void tracy_xfree(void* p, const char* file, uint32_t line, const char* func);

void* tracy_xrealloc(void* p, size_t n, const char* file, uint32_t line, const char* func);

void* tracy_xcalloc(size_t count, size_t size, const char* file, uint32_t line, const char* func);

void* tracy_xmalloc_dbg(size_t n, const char* file, uint32_t line, const char* func);

void  tracy_xfree_dbg(void* p, const char* file, uint32_t line, const char* func);

void* tracy_xrealloc_dbg(void* p, size_t n, const char* file, uint32_t line, const char* func);

void* tracy_xcalloc_dbg(size_t count, size_t size, const char* file, uint32_t line, const char* func);

#define ALLOC_DEBUG
// #undef TRACY_MEMORY

#ifdef TRACY_MEMORY
#ifdef ALLOC_DEBUG
#define mp_malloc(sz)        tracy_xmalloc_dbg(sz, __FILE__, (uint32_t)__LINE__, __func__)
#define mp_free(ptr)         tracy_xfree_dbg(ptr, __FILE__, (uint32_t)__LINE__, __func__)
#define mp_realloc(ptr, sz)  tracy_xrealloc_dbg(ptr, sz, __FILE__, (uint32_t)__LINE__, __func__)
#define mp_calloc(cnt, sz)    tracy_xcalloc_dbg(cnt, sz, __FILE__, (uint32_t)__LINE__, __func__)
#else
#define mp_malloc(sz)        tracy_xmalloc(sz, __FILE__, (uint32_t)__LINE__, __func__)
#define mp_free(ptr)         tracy_xfree(ptr, __FILE__, (uint32_t)__LINE__, __func__)
#define mp_realloc(ptr, sz)  tracy_xrealloc(ptr, sz, __FILE__, (uint32_t)__LINE__, __func__)
#define mp_calloc(cnt, sz)    tracy_xcalloc(cnt, sz, __FILE__, (uint32_t)__LINE__, __func__)
#endif
#else
#define mp_malloc(sz)        malloc(sz)
#define mp_free(ptr)         free(ptr)
#define mp_realloc(ptr, sz)  realloc(ptr, sz)
#define mp_calloc(cnt, sz)   calloc(cnt, sz)
#endif


#endif //APEXPREDATOR_MEMORY_PROFILING_H
