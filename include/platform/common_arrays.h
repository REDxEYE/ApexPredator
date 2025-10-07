// Created by RED on 03.10.2025.

#ifndef APEXPREDATOR_COMMON_ARRAYS_H
#define APEXPREDATOR_COMMON_ARRAYS_H

#include "int_def.h"
#include "utils/dynamic_array.h"
#include "utils/string.h"

DYNAMIC_ARRAY_STRUCT(float32, float32);

DYNAMIC_ARRAY_STRUCT(uint64, uint64);

DYNAMIC_ARRAY_STRUCT(uint32, uint32);

DYNAMIC_ARRAY_STRUCT(uint8, uint8);

DYNAMIC_ARRAY_STRUCT(String, String);

DYNAMIC_ARRAY_STRUCT(char*, charPtr);

#endif //APEXPREDATOR_COMMON_ARRAYS_H
