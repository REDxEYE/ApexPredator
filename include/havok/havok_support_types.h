// Created by RED on 06.02.2026.

#ifndef APEXPREDATOR_HAVOK_SUPPORT_TYPES_H
#define APEXPREDATOR_HAVOK_SUPPORT_TYPES_H
#include "havok/havok_type_info_map.h"

typedef void *voidPtr;
typedef int8 signed_char;
typedef const int8 const_char;
typedef uint32 unsigned_int;
typedef uint16 unsigned_short;
typedef uint8 unsigned_char;
typedef uint64 unsigned_long_long;
typedef const char *const_charPtr;

typedef struct hkReflect__Type {
    HavokTypeInfo *type_info_;
} hkReflect__Type;

typedef struct hkReflect__Detail__Opaque {
    HavokTypeInfo *type_info_;
} hkReflect__Detail__Opaque;


#endif //APEXPREDATOR_HAVOK_SUPPORT_TYPES_H