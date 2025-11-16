// Created by RED on 10.10.2025.

#ifndef APEXPREDATOR_ENDIAN_H
#define APEXPREDATOR_ENDIAN_H

#if defined(_MSC_VER)
  #include <intrin.h>
#endif

#if defined(__has_builtin)
  #define _HAS_BUILTIN(x) __has_builtin(x)
#else
  #define _HAS_BUILTIN(x) 0
#endif

/* BSWAP16/32/64: byte-swap unsigned integers */
#if defined(_MSC_VER)
  #define BSWAP16(x) ((uint16_t)_byteswap_ushort((unsigned short)(x)))
  #define BSWAP32(x) ((uint32_t)_byteswap_ulong((unsigned long)(x)))
  #if defined(_M_X64) || defined(_M_ARM64) || defined(_M_AMD64) || defined(_M_IA64)
    #define BSWAP64(x) ((uint64_t)_byteswap_uint64((unsigned __int64)(x)))
  #else
    #define BSWAP64(x) ((uint64_t)_byteswap_uint64((unsigned __int64)(x)))
  #endif

#elif _HAS_BUILTIN(__builtin_bswap16) || defined(__GNUC__) || defined(__clang__)
  #define BSWAP16(x) ((uint16_t)__builtin_bswap16((uint16_t)(x)))
  #define BSWAP32(x) ((uint32_t)__builtin_bswap32((uint32_t)(x)))
  #define BSWAP64(x) ((uint64_t)__builtin_bswap64((uint64_t)(x)))

#elif defined(__GLIBC__) || defined(__GNU_LIBRARY__)
  #include <byteswap.h>
  #define BSWAP16(x) ((uint16_t)bswap_16((uint16_t)(x)))
  #define BSWAP32(x) ((uint32_t)bswap_32((uint32_t)(x)))
  #define BSWAP64(x) ((uint64_t)bswap_64((uint64_t)(x)))

#elif defined(__APPLE__)
  #include <libkern/OSByteOrder.h>
  #define BSWAP16(x) ((uint16_t)OSSwapInt16((uint16_t)(x)))
  #define BSWAP32(x) ((uint32_t)OSSwapInt32((uint32_t)(x)))
  #define BSWAP64(x) ((uint64_t)OSSwapInt64((uint64_t)(x)))

#else
  #define BSWAP16(x) ( \
      (uint16_t)((((uint16_t)(x) & UINT16_C(0x00FF)) << 8) | \
                 (((uint16_t)(x) & UINT16_C(0xFF00)) >> 8)) ) )

  #define BSWAP32(x) ( \
      (uint32_t)((((uint32_t)(x) & UINT32_C(0x000000FF)) << 24) | \
                 (((uint32_t)(x) & UINT32_C(0x0000FF00)) << 8)  | \
                 (((uint32_t)(x) & UINT32_C(0x00FF0000)) >> 8)  | \
                 (((uint32_t)(x) & UINT32_C(0xFF000000)) >> 24)) ) )

  #define BSWAP64(x) ( \
      (uint64_t)((((uint64_t)(x) & UINT64_C(0x00000000000000FF)) << 56) | \
                 (((uint64_t)(x) & UINT64_C(0x000000000000FF00)) << 40) | \
                 (((uint64_t)(x) & UINT64_C(0x0000000000FF0000)) << 24) | \
                 (((uint64_t)(x) & UINT64_C(0x00000000FF000000)) << 8)  | \
                 (((uint64_t)(x) & UINT64_C(0x000000FF00000000)) >> 8)  | \
                 (((uint64_t)(x) & UINT64_C(0x0000FF0000000000)) >> 24) | \
                 (((uint64_t)(x) & UINT64_C(0x00FF000000000000)) >> 40) | \
                 (((uint64_t)(x) & UINT64_C(0xFF00000000000000)) >> 56)) ) )
#endif

/* Host endianness detection */
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
  #define _HOST_LITTLE_ENDIAN (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#elif defined(_MSC_VER) || defined(__LITTLE_ENDIAN__) || defined(_M_IX86) || defined(_M_X64) || defined(__x86_64__) || defined(__i386__) || defined(__ARMEL__) || defined(__AARCH64EL__)
  #define _HOST_LITTLE_ENDIAN 1
#else
  #define _HOST_LITTLE_ENDIAN 0
#endif

/* Convert between host and specific endianness */
#define HTOLE16(x) ((uint16_t)(x))
#define HTOLE32(x) ((uint32_t)(x))
#define HTOLE64(x) ((uint64_t)(x))
#define HTOBE16(x) BSWAP16(x)
#define HTOBE32(x) BSWAP32(x)
#define HTOBE64(x) BSWAP64(x)
#define LE16TOH(x) ((uint16_t)(x))
#define LE32TOH(x) ((uint32_t)(x))
#define LE64TOH(x) ((uint64_t)(x))
#define BE16TOH(x) BSWAP16(x)
#define BE32TOH(x) BSWAP32(x)
#define BE64TOH(x) BSWAP64(x)

#if _HOST_LITTLE_ENDIAN
  #undef HTOLE16
  #undef HTOLE32
  #undef HTOLE64
  #undef LE16TOH
  #undef LE32TOH
  #undef LE64TOH
  #define HTOLE16(x) ((uint16_t)(x))
  #define HTOLE32(x) ((uint32_t)(x))
  #define HTOLE64(x) ((uint64_t)(x))
  #define LE16TOH(x) ((uint16_t)(x))
  #define LE32TOH(x) ((uint32_t)(x))
  #define LE64TOH(x) ((uint64_t)(x))
  #undef HTOBE16
  #undef HTOBE32
  #undef HTOBE64
  #undef BE16TOH
  #undef BE32TOH
  #undef BE64TOH
  #define HTOBE16(x) BSWAP16(x)
  #define HTOBE32(x) BSWAP32(x)
  #define HTOBE64(x) BSWAP64(x)
  #define BE16TOH(x) BSWAP16(x)
  #define BE32TOH(x) BSWAP32(x)
  #define BE64TOH(x) BSWAP64(x)
#else
  #undef HTOLE16
  #undef HTOLE32
  #undef HTOLE64
  #undef LE16TOH
  #undef LE32TOH
  #undef LE64TOH
  #define HTOLE16(x) BSWAP16(x)
  #define HTOLE32(x) BSWAP32(x)
  #define HTOLE64(x) BSWAP64(x)
  #define LE16TOH(x) BSWAP16(x)
  #define LE32TOH(x) BSWAP32(x)
  #define LE64TOH(x) BSWAP64(x)
  #undef HTOBE16
  #undef HTOBE32
  #undef HTOBE64
  #undef BE16TOH
  #undef BE32TOH
  #undef BE64TOH
  #define HTOBE16(x) ((uint16_t)(x))
  #define HTOBE32(x) ((uint32_t)(x))
  #define HTOBE64(x) ((uint64_t)(x))
  #define BE16TOH(x) ((uint16_t)(x))
  #define BE32TOH(x) ((uint32_t)(x))
  #define BE64TOH(x) ((uint64_t)(x))
#endif
#endif //APEXPREDATOR_ENDIAN_H