// bf_hash_mt_simd_validate.c
// Brute-force Jenkins lookup3 hashlittle (canonical 'c') with AVX-512/AVX2 and scalar fallback.
// Prints ALL matches, and VALIDATES each match using the original scalar hashlittle.
// Created: 2025-10-04

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <process.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <immintrin.h>
#include <stdalign.h>

/* ------------ Config ------------ */
#define VALIDATE_MATCHES 1  /* Always re-check matches with scalar hashlittle before printing */

/* ------------ Print lock (avoid interleaved lines) ------------ */
static CRITICAL_SECTION g_print_cs;
#define PRINTF_LOCKED(...) do { EnterCriticalSection(&g_print_cs); printf(__VA_ARGS__); LeaveCriticalSection(&g_print_cs); } while(0)

/* ------------ Scalar reference (original lookup3 behavior) ------------ */
static inline uint32_t rot(uint32_t x, int k) { return (x << k) | (x >> (32 - k)); }

static inline void mix_scalar(uint32_t *a, uint32_t *b, uint32_t *c) {
    *a -= *c;  *a ^= rot(*c, 4);  *c += *b;
    *b -= *a;  *b ^= rot(*a, 6);  *a += *c;
    *c -= *b;  *c ^= rot(*b, 8);  *b += *a;
    *a -= *c;  *a ^= rot(*c,16);  *c += *b;
    *b -= *a;  *b ^= rot(*a,19);  *a += *c;
}
static inline void final_scalar(uint32_t *a, uint32_t *b, uint32_t *c) {
    *c ^= *b; *c -= rot(*b,14);
    *a ^= *c; *a -= rot(*c,11);
    *b ^= *a; *b -= rot(*a,25);
    *c ^= *b; *c -= rot(*b,16);
    *a ^= *c; *a -= rot(*c,4);
    *b ^= *a; *b -= rot(*a,14);
    *c ^= *b; *c -= rot(*b,24);
}

static void hashlittle_abc_scalar(const void *key, size_t length, uint32_t initval,
                                  uint32_t *out_a, uint32_t *out_b, uint32_t *out_c)
{
    const uint8_t *k = (const uint8_t*)key;
    uint32_t a, b, c;
    a = b = c = 0xdeadbeef + (uint32_t)length + initval;

    while (length > 12) {
        a += (uint32_t)k[0]  | ((uint32_t)k[1] << 8) | ((uint32_t)k[2] << 16) | ((uint32_t)k[3] << 24);
        b += (uint32_t)k[4]  | ((uint32_t)k[5] << 8) | ((uint32_t)k[6] << 16) | ((uint32_t)k[7] << 24);
        c += (uint32_t)k[8]  | ((uint32_t)k[9] << 8) | ((uint32_t)k[10] << 16) | ((uint32_t)k[11] << 24);
        mix_scalar(&a, &b, &c);
        k += 12; length -= 12;
    }

    switch (length) {
    case 12: c += ((uint32_t)k[11]) << 24;
    case 11: c += ((uint32_t)k[10]) << 16;
    case 10: c += ((uint32_t)k[ 9]) << 8;
    case  9: c += ((uint32_t)k[ 8]);
    case  8: b += ((uint32_t)k[ 7]) << 24;
    case  7: b += ((uint32_t)k[ 6]) << 16;
    case  6: b += ((uint32_t)k[ 5]) << 8;
    case  5: b += ((uint32_t)k[ 4]);
    case  4: a += ((uint32_t)k[ 3]) << 24;
    case  3: a += ((uint32_t)k[ 2]) << 16;
    case  2: a += ((uint32_t)k[ 1]) << 8;
    case  1: a += ((uint32_t)k[ 0]);
    default: break;
    }

    final_scalar(&a, &b, &c);
    if (out_a) *out_a = a; if (out_b) *out_b = b; if (out_c) *out_c = c;
}

static inline uint32_t hashlittle_c_scalar(const void *key, size_t length, uint32_t initval) {
    uint32_t a,b,c; hashlittle_abc_scalar(key,length,initval,&a,&b,&c); return c;
}

/* classic API name */
static inline uint32_t hashlittle(const void *key, size_t length, uint32_t initval) {
    return hashlittle_c_scalar(key, length, initval);
}

/* ------------ AVX2: 8-lane SIMD (returns 'c') ------------ */
#if defined(__AVX2__)
static inline __m256i rol32_256(__m256i x, int k) {
    __m256i sl = _mm256_slli_epi32(x, k);
    __m256i sr = _mm256_srli_epi32(x, 32 - k);
    return _mm256_or_si256(sl, sr);
}

/* null-safe, equal-length messages */
static void hashlittle_c_avx2_fixedlen8(const uint8_t *const key[8], size_t length, uint32_t initval, uint32_t out_c[8]) {
    const uint32_t init = 0xdeadbeefu + (uint32_t)length + initval;
    __m256i A = _mm256_set1_epi32((int)init), B = A, C = A;

    const uint8_t *p[8]; unsigned mask = 0;
    for (int i=0;i<8;++i){ p[i]=key[i]; if(p[i]) mask|=(1u<<i); }

    size_t len = length;
    while (len > 12) {
        uint32_t w0[8], w1[8], w2[8];
        for (int i=0;i<8;++i){
            if (p[i]) {
                const uint8_t *q=p[i];
                w0[i]=(uint32_t)q[0]|((uint32_t)q[1]<<8)|((uint32_t)q[2]<<16)|((uint32_t)q[3]<<24);
                w1[i]=(uint32_t)q[4]|((uint32_t)q[5]<<8)|((uint32_t)q[6]<<16)|((uint32_t)q[7]<<24);
                w2[i]=(uint32_t)q[8]|((uint32_t)q[9]<<8)|((uint32_t)q[10]<<16)|((uint32_t)q[11]<<24);
                p[i]+=12;
            } else w0[i]=w1[i]=w2[i]=0;
        }
        __m256i W0=_mm256_loadu_si256((const __m256i*)w0);
        __m256i W1=_mm256_loadu_si256((const __m256i*)w1);
        __m256i W2=_mm256_loadu_si256((const __m256i*)w2);

        A=_mm256_add_epi32(A,W0); B=_mm256_add_epi32(B,W1); C=_mm256_add_epi32(C,W2);

        A=_mm256_sub_epi32(A,C); A=_mm256_xor_si256(A,rol32_256(C,4));  C=_mm256_add_epi32(C,B);
        B=_mm256_sub_epi32(B,A); B=_mm256_xor_si256(B,rol32_256(A,6));  A=_mm256_add_epi32(A,C);
        C=_mm256_sub_epi32(C,B); C=_mm256_xor_si256(C,rol32_256(B,8));  B=_mm256_add_epi32(B,A);
        A=_mm256_sub_epi32(A,C); A=_mm256_xor_si256(A,rol32_256(C,16)); C=_mm256_add_epi32(C,B);
        B=_mm256_sub_epi32(B,A); B=_mm256_xor_si256(B,rol32_256(A,19)); A=_mm256_add_epi32(A,C);

        len -= 12;
    }

    uint32_t ta[8]={0},tb[8]={0},tc[8]={0};
    if (len){
        for (int i=0;i<8;++i){
            if (!p[i]) continue; const uint8_t *k=p[i];
            switch(len){
            case 12: tc[i]+=(uint32_t)k[11]<<24;
            case 11: tc[i]+=(uint32_t)k[10]<<16;
            case 10: tc[i]+=(uint32_t)k[9] <<8;
            case  9: tc[i]+=(uint32_t)k[8];
            case  8: tb[i]+=(uint32_t)k[7] <<24;
            case  7: tb[i]+=(uint32_t)k[6] <<16;
            case  6: tb[i]+=(uint32_t)k[5] <<8;
            case  5: tb[i]+=(uint32_t)k[4];
            case  4: ta[i]+=(uint32_t)k[3] <<24;
            case  3: ta[i]+=(uint32_t)k[2] <<16;
            case  2: ta[i]+=(uint32_t)k[1] <<8;
            case  1: ta[i]+=(uint32_t)k[0];
            default: break;
            }
        }
    }

    __m256i TA=_mm256_loadu_si256((const __m256i*)ta);
    __m256i TB=_mm256_loadu_si256((const __m256i*)tb);
    __m256i TC=_mm256_loadu_si256((const __m256i*)tc);

    A=_mm256_add_epi32(A,TA); B=_mm256_add_epi32(B,TB); C=_mm256_add_epi32(C,TC);

    C=_mm256_xor_si256(C,B); C=_mm256_sub_epi32(C,rol32_256(B,14));
    A=_mm256_xor_si256(A,C); A=_mm256_sub_epi32(A,rol32_256(C,11));
    B=_mm256_xor_si256(B,A); B=_mm256_sub_epi32(B,rol32_256(A,25));
    C=_mm256_xor_si256(C,B); C=_mm256_sub_epi32(C,rol32_256(B,16));
    A=_mm256_xor_si256(A,C); A=_mm256_sub_epi32(A,rol32_256(C,4));
    B=_mm256_xor_si256(B,A); B=_mm256_sub_epi32(B,rol32_256(A,14));
    C=_mm256_xor_si256(C,B); C=_mm256_sub_epi32(C,rol32_256(B,24));

    alignas(32) uint32_t tmp[8];
    _mm256_storeu_si256((__m256i*)tmp, C);
    for (int i=0;i<8;++i) out_c[i]=(mask&(1u<<i))?tmp[i]:0u;
}
#endif

/* ------------ AVX-512: 16-lane SIMD (returns 'c') ------------ */
#if defined(__AVX512F__)
static inline __m512i rol32_512(__m512i x, int k) {
#if defined(_mm512_rol_epi32)
    return _mm512_rol_epi32(x,k);
#else
    __m512i sl=_mm512_slli_epi32(x,k), sr=_mm512_srli_epi32(x,32-k);
    return _mm512_or_si512(sl,sr);
#endif
}

/* null-safe, equal-length messages */
static void hashlittle_c_avx512_fixedlen16(const uint8_t *const key[16], size_t length, uint32_t initval, uint32_t out_c[16]) {
    const uint32_t init = 0xdeadbeefu + (uint32_t)length + initval;
    __m512i A=_mm512_set1_epi32((int)init), B=A, C=A;

    const uint8_t *p[16]; uint32_t mask=0;
    for (int i=0;i<16;++i){ p[i]=key[i]; if(p[i]) mask|=(1u<<i); }

    size_t len = length;
    while (len > 12) {
        uint32_t w0[16], w1[16], w2[16];
        for (int i=0;i<16;++i){
            if (p[i]) {
                const uint8_t *q=p[i];
                w0[i]=(uint32_t)q[0]|((uint32_t)q[1]<<8)|((uint32_t)q[2]<<16)|((uint32_t)q[3]<<24);
                w1[i]=(uint32_t)q[4]|((uint32_t)q[5]<<8)|((uint32_t)q[6]<<16)|((uint32_t)q[7]<<24);
                w2[i]=(uint32_t)q[8]|((uint32_t)q[9]<<8)|((uint32_t)q[10]<<16)|((uint32_t)q[11]<<24);
                p[i]+=12;
            } else w0[i]=w1[i]=w2[i]=0;
        }
        __m512i W0=_mm512_loadu_si512((const void*)w0);
        __m512i W1=_mm512_loadu_si512((const void*)w1);
        __m512i W2=_mm512_loadu_si512((const void*)w2);

        A=_mm512_add_epi32(A,W0); B=_mm512_add_epi32(B,W1); C=_mm512_add_epi32(C,W2);

        A=_mm512_sub_epi32(A,C); A=_mm512_xor_epi32(A,rol32_512(C,4));  C=_mm512_add_epi32(C,B);
        B=_mm512_sub_epi32(B,A); B=_mm512_xor_epi32(B,rol32_512(A,6));  A=_mm512_add_epi32(A,C);
        C=_mm512_sub_epi32(C,B); C=_mm512_xor_epi32(C,rol32_512(B,8));  B=_mm512_add_epi32(B,A);
        A=_mm512_sub_epi32(A,C); A=_mm512_xor_epi32(A,rol32_512(C,16)); C=_mm512_add_epi32(C,B);
        B=_mm512_sub_epi32(B,A); B=_mm512_xor_epi32(B,rol32_512(A,19)); A=_mm512_add_epi32(A,C);

        len -= 12;
    }

    uint32_t ta[16]={0},tb[16]={0},tc[16]={0};
    if (len){
        for (int i=0;i<16;++i){
            if (!p[i]) continue; const uint8_t *k=p[i];
            switch(len){
            case 12: tc[i]+=(uint32_t)k[11]<<24;
            case 11: tc[i]+=(uint32_t)k[10]<<16;
            case 10: tc[i]+=(uint32_t)k[9] <<8;
            case  9: tc[i]+=(uint32_t)k[8];
            case  8: tb[i]+=(uint32_t)k[7] <<24;
            case  7: tb[i]+=(uint32_t)k[6] <<16;
            case  6: tb[i]+=(uint32_t)k[5] <<8;
            case  5: tb[i]+=(uint32_t)k[4];
            case  4: ta[i]+=(uint32_t)k[3] <<24;
            case  3: ta[i]+=(uint32_t)k[2] <<16;
            case  2: ta[i]+=(uint32_t)k[1] <<8;
            case  1: ta[i]+=(uint32_t)k[0];
            default: break;
            }
        }
    }

    __m512i TA=_mm512_loadu_si512((const void*)ta);
    __m512i TB=_mm512_loadu_si512((const void*)tb);
    __m512i TC=_mm512_loadu_si512((const void*)tc);

    A=_mm512_add_epi32(A,TA); B=_mm512_add_epi32(B,TB); C=_mm512_add_epi32(C,TC);

    C=_mm512_xor_epi32(C,B); C=_mm512_sub_epi32(C,rol32_512(B,14));
    A=_mm512_xor_epi32(A,C); A=_mm512_sub_epi32(A,rol32_512(C,11));
    B=_mm512_xor_epi32(B,A); B=_mm512_sub_epi32(B,rol32_512(A,25));
    C=_mm512_xor_epi32(C,B); C=_mm512_sub_epi32(C,rol32_512(B,16));
    A=_mm512_xor_epi32(A,C); A=_mm512_sub_epi32(A,rol32_512(C,4));
    B=_mm512_xor_epi32(B,A); B=_mm512_sub_epi32(B,rol32_512(A,14));
    C=_mm512_xor_epi32(C,B); C=_mm512_sub_epi32(C,rol32_512(B,24));

    alignas(64) uint32_t tmp[16];
    _mm512_storeu_si512((void*)tmp, C);
    for (int i=0;i<16;++i) out_c[i]=(mask&(1u<<i))?tmp[i]:0u;
}
#endif

/* ------------ Worker & MT driver ------------ */
typedef struct {
    const char *charset;
    size_t charset_len;
    size_t len;
    uint32_t target;
    unsigned thread_id;
    unsigned thread_count;
} worker_arg_t;

static unsigned __stdcall worker_thread(void *argv) {
    worker_arg_t *arg = (worker_arg_t*)argv;
    const char *charset = arg->charset;
    const size_t charset_len = arg->charset_len;
    const size_t len = arg->len;
    const uint32_t target = arg->target;
    const unsigned tid = arg->thread_id;
    const unsigned tcount = arg->thread_count;

#if defined(__AVX512F__)
    const size_t LANES = 16;
#elif defined(__AVX2__)
    const size_t LANES = 8;
#else
    const size_t LANES = 1;
#endif

    if (len == 0) return 0;
    const size_t tail_len = (len > 1) ? (len - 1) : 0;

    /* lane buffers */
    uint8_t **lane_bufs = (uint8_t**)malloc(sizeof(uint8_t*) * LANES);
    if (!lane_bufs) return 0;
    for (size_t i = 0; i < LANES; ++i) {
        lane_bufs[i] = (uint8_t*)malloc(len + 1);
        if (!lane_bufs[i]) { for (size_t j=0;j<i;++j) free(lane_bufs[j]); free(lane_bufs); return 0; }
        lane_bufs[i][len] = 0;
    }

    size_t *tail_idx = NULL;
    if (tail_len) {
        tail_idx = (size_t*)calloc(tail_len, sizeof(size_t));
        if (!tail_idx) { for (size_t i=0;i<LANES;++i) free(lane_bufs[i]); free(lane_bufs); return 0; }
    }

    for (size_t first = tid; first < charset_len; first += tcount) {
        for (size_t li=0; li<LANES; ++li) lane_bufs[li][0] = (uint8_t)charset[first];

        if (len == 1) {
            uint32_t c = hashlittle(lane_bufs[0], 1, 0);
#if VALIDATE_MATCHES
            if (c == target) PRINTF_LOCKED("MATCH: \"%s\"  c=%08" PRIX32 " [validated]\n", (char*)lane_bufs[0], c);
#else
            if (c == target) PRINTF_LOCKED("MATCH: \"%s\"  c=%08" PRIX32 "\n", (char*)lane_bufs[0], c);
#endif
            continue;
        }

        if (tail_idx) memset(tail_idx, 0, tail_len * sizeof(size_t));

        while (1) {
            size_t filled = 0;
            for (; filled < LANES; ++filled) {
                for (size_t pos=0; pos<tail_len; ++pos) {
                    lane_bufs[filled][pos+1] = (uint8_t)charset[ tail_idx[pos] ];
                }
                /* advance odometer */
                size_t pos = 0;
                while (pos < tail_len) {
                    tail_idx[pos]++;
                    if (tail_idx[pos] < charset_len) break;
                    tail_idx[pos] = 0; pos++;
                }
                if (pos == tail_len) { filled++; break; } /* last batch */
            }

            if (filled == 0) break;

#if defined(__AVX512F__)
            {
                const uint8_t *kptrs[16] = {0};
                for (size_t j=0; j<filled; ++j) kptrs[j] = lane_bufs[j];
                uint32_t out_c[16] = {0};
                hashlittle_c_avx512_fixedlen16(kptrs, len, 0, out_c);
                for (size_t j=0; j<filled; ++j) {
                    if (out_c[j] == target) {
#if VALIDATE_MATCHES
                        uint32_t v = hashlittle(lane_bufs[j], len, 0);
                        if (v == target)
                            PRINTF_LOCKED("MATCH: \"%s\"  c=%08" PRIX32 " [validated]\n", (char*)lane_bufs[j], v);
                        else
                            PRINTF_LOCKED("MISMATCH: SIMD=%08" PRIX32 " scalar=%08" PRIX32 " for \"%s\"\n", out_c[j], v, (char*)lane_bufs[j]);
#else
                        PRINTF_LOCKED("MATCH: \"%s\"  c=%08" PRIX32 "\n", (char*)lane_bufs[j], out_c[j]);
#endif
                    }
                }
            }
#elif defined(__AVX2__)
            {
                const uint8_t *kptrs[8] = {0};
                for (size_t j=0; j<filled && j<8; ++j) kptrs[j] = lane_bufs[j];
                uint32_t out_c[8] = {0};
                hashlittle_c_avx2_fixedlen8(kptrs, len, 0, out_c);
                for (size_t j=0; j<filled && j<8; ++j) {
                    if (out_c[j] == target) {
#if VALIDATE_MATCHES
                        uint32_t v = hashlittle(lane_bufs[j], len, 0);
                        if (v == target)
                            PRINTF_LOCKED("MATCH: \"%s\"  c=%08" PRIX32 " [validated]\n", (char*)lane_bufs[j], v);
                        else
                            PRINTF_LOCKED("MISMATCH: SIMD=%08" PRIX32 " scalar=%08" PRIX32 " for \"%s\"\n", out_c[j], v, (char*)lane_bufs[j]);
#else
                        PRINTF_LOCKED("MATCH: \"%s\"  c=%08" PRIX32 "\n", (char*)lane_bufs[j], out_c[j]);
#endif
                    }
                }
            }
#else
            {
                for (size_t j=0; j<filled; ++j) {
                    uint32_t c = hashlittle(lane_bufs[j], len, 0);
#if VALIDATE_MATCHES
                    if (c == target)
                        PRINTF_LOCKED("MATCH: \"%s\"  c=%08" PRIX32 " [validated]\n", (char*)lane_bufs[j], c);
#else
                    if (c == target)
                        PRINTF_LOCKED("MATCH: \"%s\"  c=%08" PRIX32 "\n", (char*)lane_bufs[j], c);
#endif
                }
            }
#endif
            if (filled < LANES) break; /* odometer wrapped */
        } // while odometer
    } // for first

    if (tail_idx) free(tail_idx);
    for (size_t i=0;i<LANES;++i) free(lane_bufs[i]);
    free(lane_bufs);
    return 0;
}

static unsigned get_logical_cpu_count(void) { SYSTEM_INFO si; GetSystemInfo(&si); return si.dwNumberOfProcessors ? si.dwNumberOfProcessors : 1; }

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <target-hex> [maxlen] [charset] [threads]\n", argv[0]);
        fprintf(stderr, "Example: %s 0xD31AB684 6 abcdefghijklmnopqrstuvwxyz0123456789_ 8\n", argv[0]);
        return 1;
    }

    InitializeCriticalSection(&g_print_cs);

    uint32_t target = (uint32_t)strtoul(argv[1], NULL, 0);
    int maxlen = (argc >= 3) ? atoi(argv[2]) : 6;
    const char *charset = (argc >= 4) ? argv[3] : "abcdefghijklmnopqrstuvwxyz0123456789_";
    unsigned requested_threads = (argc >= 5) ? (unsigned)atoi(argv[4]) : 0;

    size_t charset_len = strlen(charset);
    if (charset_len == 0) { fprintf(stderr, "Empty charset\n"); DeleteCriticalSection(&g_print_cs); return 1; }
    if (maxlen <= 0 || maxlen > 64) { fprintf(stderr, "maxlen must be 1..64\n"); DeleteCriticalSection(&g_print_cs); return 1; }

    unsigned sys_cpus = get_logical_cpu_count();
    unsigned thread_count = requested_threads ? requested_threads : sys_cpus;
    if (thread_count == 0) thread_count = 1;

    printf("Target: 0x%08" PRIX32 ", charset size: %zu, maxlen: %d, threads: %u\n",
           target, charset_len, maxlen, thread_count);

    for (int len = 1; len <= maxlen; ++len) {
        printf("Trying length %d...\n", len);

        worker_arg_t *args = (worker_arg_t*)malloc(sizeof(worker_arg_t) * thread_count);
        uintptr_t *handles = (uintptr_t*)malloc(sizeof(uintptr_t) * thread_count);
        if (!args || !handles) {
            fprintf(stderr, "Out of memory\n");
            free(args); free(handles);
            DeleteCriticalSection(&g_print_cs);
            return 1;
        }

        for (unsigned t = 0; t < thread_count; ++t) {
            args[t].charset = charset;
            args[t].charset_len = charset_len;
            args[t].len = (size_t)len;
            args[t].target = target;
            args[t].thread_id = t;
            args[t].thread_count = thread_count;

            handles[t] = _beginthreadex(NULL, 0, worker_thread, &args[t], 0, NULL);
            if (handles[t] == 0) { fprintf(stderr, "Failed to create thread %u\n", t); thread_count = t; break; }
        }

        for (unsigned t = 0; t < thread_count; ++t) {
            if (handles[t]) { WaitForSingleObject((HANDLE)handles[t], INFINITE); CloseHandle((HANDLE)handles[t]); }
        }

        free(args);
        free(handles);

        printf("Finished length %d.\n", len);
    }

    printf("Scan complete.\n");
    DeleteCriticalSection(&g_print_cs);
    return 0;
}
