// Created by RED on 04.02.2026.

#include "apex/adf/adf_support_types.h"

#include <assert.h>

#include "apex/hashes.h"
#include "apex/adf/adf_types.h"
#include "apex/adf/adf_type_info_map.h"
#include "platform/logger.h"

bool int8_read(int8 *out, Buffer *buffer) {
    return buffer->read_int8(buffer, out) == BUFFER_SUCCESS;
}

bool uint8_read(uint8 *out, Buffer *buffer) {
    return buffer->read_uint8(buffer, out) == BUFFER_SUCCESS;
}

bool int16_read(int16 *out, Buffer *buffer) {
    return buffer->read_int16(buffer, out) == BUFFER_SUCCESS;
}

bool uint16_read(uint16 *out, Buffer *buffer) {
    return buffer->read_uint16(buffer, out) == BUFFER_SUCCESS;
}

bool int32_read(int32 *out, Buffer *buffer) {
    return buffer->read_int32(buffer, out) == BUFFER_SUCCESS;
}

bool uint32_read(uint32 *out, Buffer *buffer) {
    return buffer->read_uint32(buffer, out) == BUFFER_SUCCESS;
}

bool StringHash_48c5294d_4_read(StringHash_48c5294d_4 *out, Buffer *buffer) {
    return buffer->read_uint32(buffer, out) == BUFFER_SUCCESS;
}

bool StringHash_99cfa095_6_read(StringHash_99cfa095_6 *out, Buffer *buffer) {
    return buffer->read(buffer, out, 6, NULL) == BUFFER_SUCCESS;
}

bool StringHash_48c5294d_8_read(StringHash_48c5294d_8 *out, Buffer *buffer) {
    return buffer->read_uint64(buffer, out) == BUFFER_SUCCESS;
}

bool int64_read(int64 *out, Buffer *buffer) {
    return buffer->read_int64(buffer, out) == BUFFER_SUCCESS;
}

bool uint64_read(uint64 *out, Buffer *buffer) {
    return buffer->read_uint64(buffer, out) == BUFFER_SUCCESS;
}

bool float32_read(float32 *out, Buffer *buffer) {
    return buffer->read_float(buffer, out) == BUFFER_SUCCESS;
}

bool float64_read(float64 *out, Buffer *buffer) {
    return buffer->read_double(buffer, out) == BUFFER_SUCCESS;
}

bool String_read(STI_String *out, Buffer *buffer) {
    uint32 offset;
    uint32 unk;
    if (buffer->read_uint32(buffer, &offset) != BUFFER_SUCCESS) {
        return false;
    }
    if (buffer->read_uint32(buffer, &unk) != BUFFER_SUCCESS) {
        return false;
    }

    int64 ooffset = 0;
    buffer->get_position(buffer, &ooffset);
    buffer->set_position(buffer, offset, BUFFER_ORIGIN_START);

    if (buffer->read_cstring(buffer, out) != BUFFER_SUCCESS) {
        if (buffer->set_position(buffer, ooffset, BUFFER_ORIGIN_START) != BUFFER_SUCCESS) {
            ;
            GLog_Error("Failed to restore buffer position after failed string read");
        }
        return false;
    }
    if (buffer->set_position(buffer, ooffset, BUFFER_ORIGIN_START) != BUFFER_SUCCESS) {
        GLog_Error("Failed to restore buffer position after string read");
        return false;
    }
    return true;
}

typedef struct DumpType {
    const STITypeInfo *type_info_;
    uint32 type_hash;
    uint8 *data;
    uint32 size;
} DumpType;

void dump_array_init(DumpType *obj);

void dump_array_free(DumpType *obj);

void dump_array_print(const DumpType *obj, JsonContext *ctx);

static const STITypeInfo dump_array = {
    .init = (initSTIObject) dump_array_init,
    .read = NULL,
    .free = (freeSTIObject) dump_array_free,
    .print = (printSTIObject) dump_array_print,
    .size = 0,
    .disk_size = 0,
    .is_struct = 0,
    .is_array = 1,
    .hash = 0xFFFFFFFF,
    .name = "Unknown type dump"
};

inline void dump_array_init(DumpType *obj) {
    obj->type_info_ = &dump_array;
}

inline void dump_array_free(DumpType *obj) {
    mp_free(obj->data);
    obj->data = NULL;
}

inline void dump_array_print(const DumpType *obj, JsonContext *ctx) {
    String tmp = {};
    String_init(&tmp, obj->size * 3 - 1);
    for (uint32 i = 0; i < obj->size; ++i) {
        String_append_format(&tmp, "%02X", obj->data[i]);
        if (i != obj->size - 1) {
            String_append_cstr(&tmp, " ");
        }
    }
    jsonBeginObject(ctx);
    jsonName(ctx, "type_hash");
    jsonValueNum(ctx, obj->type_hash);
    jsonName(ctx, "data");
    jsonValueStr(ctx, String_cstr(&tmp));
    jsonEndObject(ctx);
    String_free(&tmp);
}


bool Deferred_read(Deferred *out, Buffer *buffer) {
    const bool res = buffer->read(buffer, &out->offset, 16, NULL) == BUFFER_SUCCESS;
    if (out->offset == 0 && out->type_hash == 0) {
        return true;
    }
    const STITypeInfo **inner_type_ptr = (const STITypeInfo **)DM_get(&ADF_TYPES_type_info, out->type_hash);
    if (inner_type_ptr == NULL) {
        out->type_info_ = &dump_array;
        DumpType *dump = (DumpType *)mp_calloc(sizeof(DumpType), 1);
        out->data = dump;
        dump->data = (uint8*)mp_calloc(out->size, 1);
        dump->size = out->size;
        dump->type_hash = out->type_hash;
        int64 ooffset = 0;
        buffer->get_position(buffer, &ooffset);
        buffer->set_position(buffer, out->offset, BUFFER_ORIGIN_START);
        if (buffer->read(buffer, dump->data, out->size, NULL) != BUFFER_SUCCESS) {
            GLog_Error("Failed to read unknown deferred type hash 0x%08X", out->type_hash);
            mp_free(dump->data);
            mp_free(dump);
            return false;
        }
        buffer->set_position(buffer, ooffset, BUFFER_ORIGIN_START);

        GLog_Error("Unknown deferred type hash 0x%08X", out->type_hash);
        return false;
    }
    const STITypeInfo *inner_type = *inner_type_ptr;
    out->type_info_ = inner_type;
    out->data = mp_calloc(inner_type->size, 1);
    int64 ooffset = 0;
    buffer->get_position(buffer, &ooffset);
    buffer->set_position(buffer, out->offset, BUFFER_ORIGIN_START);
    if (inner_type->init == NULL) {
        GLog_Error("Inner type of deferred type hash 0x%08X does not have init function", out->type_hash);
        abort();
    }
    inner_type->init(out->data);
    if (!inner_type->read(out->data, buffer)) {
        GLog_Error("Failed to read deferred type hash 0x%08X", out->type_hash);
        return false;
    }

    buffer->set_position(buffer, ooffset, BUFFER_ORIGIN_START);
    return res;
}

void STI_String_free(String *obj) {
    String_free(obj);
}

void Deferred_free(Deferred *obj) {
    if (obj->offset == 0 && obj->type_hash == 0) {
        return;
    }
    if (obj->type_info_ == NULL) {
        GLog_Error("Deferred object has non-zero offset and type hash but no type info");
        return;
    }
    if (obj->type_info_->free != NULL) {
        obj->type_info_->free(obj->data);
    }
    mp_free(obj->data);
}

void Deferred_init(Deferred *obj) {
}

void int8_print(const int8 *obj, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

void uint8_print(const uint8 *obj, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

void int16_print(const int16 *obj, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

void uint16_print(const uint16 *obj, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

void int32_print(const int32 *obj, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

void uint32_print(const uint32 *obj, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

void StringHash_48c5294d_4_print(const StringHash_48c5294d_4 *obj, JsonContext *ctx) {
    const StringView string = find_name32_sv(*obj);
    if (sv_is_null(string)) {
        static char buffer[64];
        snprintf(buffer, sizeof(buffer), "0x%08X", *obj);
        jsonValueStr(ctx, buffer);
    }
    else {
        jsonValueStr(ctx, StringView_cstr(string));
    }
}

void StringHash_99cfa095_6_print(const StringHash_99cfa095_6 *obj, JsonContext *ctx) {
    const StringView string = find_name64_sv(*obj);
    if (sv_is_null(string)) {
        static char buffer[64];
        snprintf(buffer, sizeof(buffer), "0x%012llX", (unsigned long long) *obj);
        jsonValueStr(ctx, buffer);
    }
    else {
        jsonValueStr(ctx, StringView_cstr(string));
    }
}

void StringHash_48c5294d_8_print(const StringHash_48c5294d_8 *obj, JsonContext *ctx) {
    const StringView string = find_name64_sv(*obj);
    if (sv_is_null(string)) {
        static char buffer[64];
        snprintf(buffer, sizeof(buffer), "0x%016llX", (unsigned long long) *obj);
        jsonValueStr(ctx, buffer);
    }
    else {
        jsonValueStr(ctx, StringView_cstr(string));
    }
}

void int64_print(const int64 *obj, JsonContext *ctx) {
    jsonValueNum(ctx, *obj);
}

void uint64_print(const uint64 *obj, JsonContext *ctx) {
    jsonValueNum(ctx, (int64)*obj);
}

void float32_print(const float32 *obj, JsonContext *ctx) {
    jsonValueFlt(ctx, *obj);
}

void float64_print(const float64 *obj, JsonContext *ctx) {
    jsonValueFlt(ctx, *obj);
}

void String_print(const STI_String *obj, JsonContext *ctx) {
    jsonValueStr(ctx, String_cstr(obj));
}

void Deferred_print(const Deferred *obj, JsonContext *ctx) {
    if (obj->offset == 0 && obj->type_hash == 0) {
        jsonValueNull(ctx);
        return;
    }
    if (obj->type_info_ == NULL) {
        GLog_Error("Deferred object has non-zero offset and type hash but no type info");
        jsonValueNull(ctx);
        return;
    }
    if (obj->type_info_->print != NULL) {
        obj->type_info_->print(obj->data, ctx);
    }
}
