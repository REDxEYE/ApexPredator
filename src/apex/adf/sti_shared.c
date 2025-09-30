// Created by RED on 21.09.2025.

#include "apex/adf/sti_shared.h"

#include <stdlib.h>

#include "apex/adf/sti.h"

bool read_STI_int8(Buffer *buffer, STI_TypeLibrary *lib, STI_int8 *out) {
    return buffer->read_int8(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_uint8(Buffer *buffer, STI_TypeLibrary *lib, STI_uint8 *out) {
    return buffer->read_uint8(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_int16(Buffer *buffer, STI_TypeLibrary *lib, STI_int16 *out) {
    return buffer->read_int16(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_uint16(Buffer *buffer, STI_TypeLibrary *lib, STI_uint16 *out) {
    return buffer->read_uint16(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_int32(Buffer *buffer, STI_TypeLibrary *lib, STI_int32 *out) {
    return buffer->read_int32(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_uint32(Buffer *buffer, STI_TypeLibrary *lib, STI_uint32 *out) {
    return buffer->read_uint32(buffer, out) == BUFFER_SUCCESS;
}

bool read_StringHash_48c5294d_4(Buffer *buffer, STI_TypeLibrary *lib, StringHash_48c5294d_4 *out) {
    return buffer->read_uint32(buffer, out) == BUFFER_SUCCESS;
}

bool read_StringHash_99cfa095_6(Buffer *buffer, STI_TypeLibrary *lib, StringHash_99cfa095_6 *out) {
    return buffer->read(buffer, out, 6, NULL) == BUFFER_SUCCESS;
}

bool read_STI_int64(Buffer *buffer, STI_TypeLibrary *lib, STI_int64 *out) {
    return buffer->read_int64(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_uint64(Buffer *buffer, STI_TypeLibrary *lib, STI_uint64 *out) {
    return buffer->read_uint64(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_float32(Buffer *buffer, STI_TypeLibrary *lib, STI_float32 *out) {
    return buffer->read_float(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_float64(Buffer *buffer, STI_TypeLibrary *lib, STI_float64 *out) {
    return buffer->read_double(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_String(Buffer *buffer, STI_TypeLibrary *lib, STI_String *out) {
    return buffer->read_cstring(buffer, out) == BUFFER_SUCCESS;
}

bool read_STI_Deferred(Buffer *buffer, STI_TypeLibrary *lib, STI_Deferred *out) {
    bool res = buffer->read(buffer, out, 16, NULL) == BUFFER_SUCCESS;
    if (out->offset == 0 && out->type_hash == 0) {
        return true;
    }
    STI_Type *inner_type = DM_get(&lib->types, out->type_hash);
    if (inner_type == NULL) {
        printf("Unknown deferred type hash 0x%08X\n", out->type_hash);
        return false;
    }
    out->data = malloc(inner_type->info.size);
    int64 ooffset = 0;
    buffer->get_position(buffer, &ooffset);
    buffer->set_position(buffer, out->offset, BUFFER_ORIGIN_START);
    STI_ObjectMethods *methods = DM_get(&lib->object_functions, out->type_hash);
    if (methods == NULL) {
        printf("No read function for deferred type hash 0x%08X\n", out->type_hash);
        return false;
    }
    if (!methods->read(buffer, lib, out->data)) {
        printf("Failed to read deferred type hash 0x%08X\n", out->type_hash);
        return false;
    }

    buffer->set_position(buffer, ooffset, BUFFER_ORIGIN_START);
    return res;
}

void free_STI_String(String *obj, STI_TypeLibrary *lib) {
    String_free(obj);
}

void free_STI_Deferred(STI_Deferred *obj, STI_TypeLibrary *lib) {
    if (obj->offset == 0 && obj->type_hash == 0) {
        return;
    }
    STI_Type *inner_type = DM_get(&lib->types, obj->type_hash);
    if (inner_type == NULL) {
        printf("Unknown deferred type hash 0x%08X\n", obj->type_hash);
        exit(1);
    }
    STI_ObjectMethods *methods = DM_get(&lib->object_functions, obj->type_hash);
    if (methods == NULL) {
        printf("No free function for deferred type hash 0x%08X\n", obj->type_hash);
        exit(1);
    }
    methods->free(obj->data, lib);
    free(obj->data);
}

void print_STI_int8(const STI_int8 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent) {
    fprintf(handle, "%i", *obj);
}

void print_STI_uint8(const STI_uint8 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent) {
    fprintf(handle, "%u", *obj);
}

void print_STI_int16(const STI_int16 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent) {
    fprintf(handle, "%i", *obj);
}

void print_STI_uint16(const STI_uint16 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent) {
    fprintf(handle, "%u", *obj);
}

void print_STI_int32(const STI_int32 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent) {
    fprintf(handle, "%i", *obj);
}

void print_STI_uint32(const STI_uint32 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent) {
    fprintf(handle, "%u", *obj);
}

void print_StringHash_48c5294d_4(const StringHash_48c5294d_4 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent) {
    String *string = DM_get(&lib->hash_strings, *obj);
    if (string != NULL) {
        fprintf(handle, "\"%s\"", String_data(string));
    } else {
        fprintf(handle, "0x%08X", *obj);
    }
}

void print_StringHash_99cfa095_6(const StringHash_99cfa095_6 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent) {
    String *string = DM_get(&lib->hash_strings, *obj);
    if (string != NULL) {
        fprintf(handle, "\"%s\" (0x%016lX)", String_data(string), (uint64)*obj);
    } else {
        fprintf(handle, "0x%016lX", (uint64)*obj);
    }
}

void print_STI_int64(const STI_int64 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent) {
    fprintf(handle, "%lli", (long long) *obj);
}

void print_STI_uint64(const STI_uint64 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent) {
    fprintf(handle, "%llu", (unsigned long long) *obj);
}

void print_STI_float32(const STI_float32 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent) {
    fprintf(handle, "%f", *obj);
}

void print_STI_float64(const STI_float64 *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent) {
    fprintf(handle, "%lf", *obj);
}

void print_STI_String(const STI_String *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent) {
    fprintf(handle, "\"%s\"", String_data(obj));
}

void print_STI_Deferred(const STI_Deferred *obj, STI_TypeLibrary *lib, FILE *handle, uint32 indent) {
    if (obj->offset == 0 && obj->type_hash == 0) {
        fprintf(handle, "<NULL>");
        return;
    }

    STI_Type *inner_type = DM_get(&lib->types, obj->type_hash);
    if (inner_type == NULL) {
        printf("Unknown deferred type hash 0x%08X\n", obj->type_hash);
        exit(1);
    }
    STI_ObjectMethods *methods = DM_get(&lib->object_functions, obj->type_hash);
    if (methods == NULL) {
        printf("No print function for deferred type hash 0x%08X\n", obj->type_hash);
        exit(1);
    }
    methods->print(obj->data, lib, handle, indent);
}
