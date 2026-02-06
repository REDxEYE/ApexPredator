// Created by RED on 19.09.2025.

#include "utils/memory_profiling.h"

#include "apex/adf/adf.h"

#include <assert.h>
#include <string.h>

#include "apex/hashes.h"
#include "platform/logger.h"
#include "utils/common.h"
#include "utils/buffer/memory_buffer.h"
#include "tracy/TracyC.h"
#include "utils/dynamic_map.h"


bool read_typedef(ADF *adf, Buffer *buffer) {
    ADFType *type = DA_append_get(&adf->types);
    buffer->read(buffer, &type->def,  sizeof(ADFTypeDef), NULL);
    const ADFTypeDef* typedef_ = &type->def;
    switch (typedef_->type) {
        case ADF_Structure: {
            uint32 member_count = 0;
            if (buffer->read_uint32(buffer, &member_count)) {
                return false;
            }
            DA_init(&type->type_data.struct_data.members, ADFStructMemberInfo, member_count);
            DA_reserve(&type->type_data.struct_data.members, member_count);
            for (int j = 0; j < member_count; ++j) {
                ADFEnumMemberInfo *member = DA_append_get(&type->type_data.struct_data.members);
                if (buffer->read(buffer, member, sizeof(ADFStructMemberInfo), NULL) != BUFFER_SUCCESS) {
                    return false;
                }
            }
            break;
        }
        case ADF_Enumeration: {
            uint32 member_count = 0;
            if (buffer->read_uint32(buffer, &member_count)) {
                return false;
            }
            DA_init(&type->type_data.enum_data.members, ADFEnumMemberInfo, member_count);
            DA_reserve(&type->type_data.enum_data.members, member_count);
            for (int j = 0; j < member_count; ++j) {
                ADFEnumMemberInfo *member = DA_append_get(&type->type_data.enum_data.members);
                if (buffer->read(buffer, member, sizeof(ADFEnumMemberInfo), NULL) != BUFFER_SUCCESS) {
                    return false;
                }
            }
            break;
        }
        case ADF_Array:
        case ADF_InlineArray: {
            if (buffer->read_uint32(buffer, &type->type_data.array_data.count)) {
                return false;
            }
            break;
        }
        case ADF_Bitfield:
        case ADF_StringHash:
        case ADF_Pointer: {
            if (buffer->read_uint32(buffer, &type->type_data.deferred_data.type_hash)) {
                return false;
            }
            break;
        }
        default: {
            GLog_Error("Unknown type %i", typedef_->type);
            assert(false && "Unknown type");
        };
    }
    return true;
}

void ADFType_free(ADFType *type) {
    switch (type->def.type) {
        case ADF_Structure: {
            DA_free(&type->type_data.struct_data.members);
            break;
        }
        case ADF_Enumeration: {
            DA_free(&type->type_data.enum_data.members);
            break;
        }
        case ADF_Primitive:
        case ADF_Bitfield:
        case ADF_Pointer:
        case ADF_StringHash:
        case ADF_Array:
        case ADF_InlineArray:
        case ADF_DeferredType: {
            break;
        }
        default: {
            GLog_Error("Unknown type %i", type->def.type);
            assert(false && "Unknown type");
        };
    }
}

bool ADF_from_buffer(ADF *adf, Buffer *buffer) {
    TracyCZoneN(ctx, "ADF_from_buffer", 1);
    ADFHeader *header = &adf->header;
    if (buffer->read(buffer, header, sizeof(ADFHeader),NULL) != BUFFER_SUCCESS) {
        TracyCZoneEnd(ctx);
        return false;
    }
    buffer->read_cstring(buffer, &adf->comment);

    buffer->set_position(buffer, header->stringhash_offset, BUFFER_ORIGIN_START);
    String hash_tmp = {0};
    for (int i = 0; i < header->stringhash_count; ++i) {
        buffer->read_cstring(buffer, &hash_tmp);
        uint64 string_hash = 0;
        buffer->read_uint64(buffer, &string_hash);
        if (check_hash64_presence(string_hash)) {
            String_free(&hash_tmp);
            continue;
        }
        store_hash64_name(string_hash, &hash_tmp);
    }
    String_free(&hash_tmp);

    DA_init(&adf->strings, String, header->nametable_count);
    buffer->set_position(buffer, header->nametable_offset + header->nametable_count, BUFFER_ORIGIN_START);
    for (int i = 0; i < header->nametable_count; ++i) {
        buffer->read_cstring(buffer,DA_append_get(&adf->strings));
    }
    buffer->set_position(buffer, header->typedef_offset, BUFFER_ORIGIN_START);
    DA_init(&adf->types, ADFType, header->typedef_count);
    for (int i = 0; i < header->typedef_count; ++i) {
        if (!read_typedef(adf, buffer)) return false;
    }
    DA_init(&adf->instances, ADFInstance, header->instance_count);
    buffer->set_position(buffer, header->instance_offset, BUFFER_ORIGIN_START);
    for (int i = 0; i < header->instance_count; ++i) {
        ADFInstance *instance = DA_append_get(&adf->instances);
        buffer->read(buffer, instance, sizeof(ADFInstance), NULL);
    }

    TracyCZoneEnd(ctx);
    return true;
}


void ADF_free(ADF *adf) {
    String_free(&adf->comment);
    DA_free_with_inner(&adf->strings, {String_free(it);});
    DA_FORI(adf->types, i) {
        ADFType_free(DA_at(&adf->types, i));
    }
    DA_free(&adf->types);
    DA_free(&adf->instances);
}

ADF* ADF_load_builtin_adf(const uint8 *data, int64 size) {
    MemoryBuffer emb = {0};
    ADF* adf = mp_calloc(sizeof(ADF), 1);
    MemoryBuffer_allocate(&emb, size);
    memcpy(emb.data, data, size);
    ADF_from_buffer(adf, (Buffer *) &emb);
    emb.close(&emb);
    return adf;
}

ADFInstance *ADF_get_instance(ADF *adf, const uint32 instance_id) {
    if (instance_id >= adf->instances.count) return NULL;
    return DA_at(&adf->instances, instance_id);
}

void *ADF_read_instance(const ADF *adf, const ADFInstance *instance, const MemoryBuffer *mb, const STITypeInfoMap* type_map) {
    TracyCZoneN(ctx, "ADF_read_instance", 1);
    const STITypeInfo **type_ptr = DM_get(type_map, instance->type_hash);
    if (type_ptr == NULL) {
        GLog_Error("Unknown type hash %08X for instance %s", instance->type_hash,
               String_cstr(&adf->strings.items[instance->name_id]));
        TracyCZoneEnd(ctx);
        return NULL;
    }
    const STITypeInfo* type = *type_ptr;

    MemoryBuffer instance_memory = {0};

    MemoryBuffer_allocate(&instance_memory, instance->size);
    memcpy(instance_memory.data, mb->data + instance->offset, instance->size);

    void *instance_data = mp_calloc(type->size, 1);
    if (type->init) {
        type->init(instance_data);
    }
    if (!type->read(instance_data, (Buffer *) &instance_memory)) {
        GLog_Error("Failed to read instance %s of type %s", String_cstr(&adf->strings.items[instance->name_id]), type->name);
        type->free(instance_data);
        instance_memory.close(&instance_memory);
        mp_free(instance_data);
        TracyCZoneEnd(ctx);
        return NULL;
    }
    instance_memory.close(&instance_memory);
    TracyCZoneEnd(ctx);
    return instance_data;
}

void ADF_free_instance(const ADFInstance *instance, void *instance_data, const STITypeInfoMap* type_map) {
    TracyCZoneN(ctx, "ADF_free_instance", 1);
    const STITypeInfo **type_ptr = DM_get(type_map, instance->type_hash);
    if (type_ptr != NULL) {

        (*type_ptr)->free(instance_data);
    }
    mp_free(instance_data);
    TracyCZoneEnd(ctx);
}

void ADF_print_instance(const ADFInstance *instance, const void *instance_data, JsonContext* ctx, const STITypeInfoMap* type_map) {
    const STITypeInfo **type_ptr = DM_get(type_map, instance->type_hash);
    if (type_ptr != NULL) {
        (*type_ptr)->print(instance_data, ctx);
    }
}
