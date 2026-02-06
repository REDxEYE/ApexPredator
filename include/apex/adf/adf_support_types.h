// Created by RED on 04.02.2026.

#ifndef APEXPREDATOR_ADF_SUPPORT_TYPES_H
#define APEXPREDATOR_ADF_SUPPORT_TYPES_H
#include <stdbool.h>

#include "sti_shared.h"
#include "utils/buffer/buffer.h"

bool int8_read(int8 *out, Buffer* buffer);

bool uint8_read(uint8 *out, Buffer* buffer);

bool int16_read(int16 *out, Buffer* buffer);

bool uint16_read(uint16 *out, Buffer* buffer);

bool int32_read(int32 *out, Buffer* buffer);

bool uint32_read(uint32 *out, Buffer* buffer);

bool StringHash_48c5294d_4_read(StringHash_48c5294d_4 *out, Buffer* buffer);

bool StringHash_99cfa095_6_read(StringHash_99cfa095_6 *out, Buffer* buffer);

bool StringHash_48c5294d_8_read(StringHash_48c5294d_8 *out, Buffer* buffer);

bool int64_read(int64 *out, Buffer* buffer);

bool uint64_read(uint64 *out, Buffer* buffer);

bool float32_read(float32 *out, Buffer* buffer);

bool float64_read(float64 *out, Buffer* buffer);

bool String_read(STI_String *out, Buffer* buffer);

bool Deferred_read(Deferred *out, Buffer* buffer);

void STI_String_free(String *obj);

void Deferred_free(Deferred *obj);

void Deferred_init(Deferred *obj);

void int8_print(const int8 *obj, JsonContext* ctx);

void uint8_print(const uint8 *obj, JsonContext* ctx);

void int16_print(const int16 *obj, JsonContext* ctx);

void uint16_print(const uint16 *obj, JsonContext* ctx);

void int32_print(const int32 *obj, JsonContext* ctx);

void uint32_print(const uint32 *obj, JsonContext* ctx);

void StringHash_48c5294d_4_print(const StringHash_48c5294d_4 *obj, JsonContext* ctx);

void StringHash_99cfa095_6_print(const StringHash_99cfa095_6 *obj, JsonContext* ctx);

void StringHash_48c5294d_8_print(const StringHash_48c5294d_8 *obj, JsonContext* ctx);

void int64_print(const int64 *obj, JsonContext* ctx);

void uint64_print(const uint64 *obj, JsonContext* ctx);

void float32_print(const float32 *obj, JsonContext* ctx);

void float64_print(const float64 *obj, JsonContext* ctx);

void String_print(const STI_String *obj, JsonContext* ctx);

void Deferred_print(const Deferred *obj, JsonContext* ctx);


#endif //APEXPREDATOR_ADF_SUPPORT_TYPES_H