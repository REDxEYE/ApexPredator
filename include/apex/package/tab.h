// Created by RED on 17.09.2025.

#ifndef APEXPREDATOR_HEADER_H
#define APEXPREDATOR_HEADER_H

#include "int_def.h"

#pragma pack(push, 1)
typedef struct {
    uint32 dwMagic; // 0x424154 (TAB\0)
    int16 wMajorVersion; // 2
    int16 wMinorVersion; // 1
    int32 dwAligment; // 2048
} TabHeader;

typedef struct {
    uint32 hash;
    uint32 offset;
    uint32 size;
}TabEntry;

#pragma pack(pop)

#endif //APEXPREDATOR_HEADER_H
