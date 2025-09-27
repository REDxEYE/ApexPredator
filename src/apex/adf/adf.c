// Created by RED on 19.09.2025.

#include "apex/adf/adf.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/common.h"
#include "utils/buffer/memory_buffer.h"


bool read_typedef(ADF *adf, Buffer *buffer, STI_TypeLibrary *lib) {
    STI_TypeDef *typedef_ = DA_append_get(&adf->type_defs);
    buffer->read(buffer, typedef_, sizeof(STI_TypeDef),NULL);
    STI_Type *type;
    bool is_dummy = false;
    if (DA_contains(&lib->types.keys, &typedef_->hash, compare_hashes)) {
        STI_Type dummy = {0};
        STI_Type_init(&dummy, typedef_->type);
        type = &dummy;
        String_copy_from(&type->name, &adf->strings.items[typedef_->name_id]);
        is_dummy=true;
    }else {
        type = STI_TypeLibrary_new_type(lib, typedef_->type, typedef_->hash, &adf->strings.items[typedef_->name_id]);
    }

    memcpy(&type->info, typedef_, sizeof(STI_TypeDef));
    // if (type->info.flags!=0x8000) {
    // }

    switch (typedef_->type) {
        case STI_Structure: {
            uint32 member_count = 0;
            if (buffer->read_uint32(buffer, &member_count)){
                return false;
            }
            DA_reserve(&type->type_data.struct_data.members, member_count);
            for (int j = 0; j < member_count; ++j) {
                STI_StructMember *member = DA_append_get(&type->type_data.struct_data.members);
                if (buffer->read(buffer, &member->info, sizeof(STI_StructMemberInfo), NULL) != BUFFER_SUCCESS) {
                    return false;
                }
                String_copy_from(&member->name, &adf->strings.items[member->info.name_id]);
            }
            break;
        }
        case STI_Enumeration: {
            uint32 member_count = 0;
            if (buffer->read_uint32(buffer, &member_count)){
                return false;
            }
            DA_reserve(&type->type_data.enum_data.members, member_count);
            for (int j = 0; j < member_count; ++j) {
                STI_EnumMember *member = DA_append_get(&type->type_data.enum_data.members);
                if (buffer->read(buffer, &member->info, sizeof(STI_EnumMemberInfo), NULL) != BUFFER_SUCCESS) {
                    return false;
                }
                String_copy_from(&member->name, &adf->strings.items[member->info.name_id]);
            }
            break;
        }
        case STI_Array:
        case STI_InlineArray: {
            if (buffer->read_uint32(buffer, &type->type_data.array_data.count)) {
                return false;
            }
            break;
        }
        case STI_Bitfield:
        case STI_StringHash:
        case STI_Pointer: {
            if (buffer->read_uint32(buffer, &type->type_data.unk_data.unk)) {
                return false;
            }
            break;
        }
        default: {
            printf("Unknown type %i\n", typedef_->type);
            assert(false && "Unknown type");
        };
    }
    if (is_dummy) {
        STI_Type_free(type);
    }
    return true;
}

bool ADF_from_buffer(ADF *adf, Buffer *buffer, STI_TypeLibrary *lib) {
    ADFHeader *header = &adf->header;
    if (buffer->read(buffer, header, sizeof(ADFHeader),NULL) != BUFFER_SUCCESS) {
        return false;
    }
    buffer->read_cstring(buffer, &adf->comment);

    buffer->set_position(buffer, header->stringhash_offset, BUFFER_ORIGIN_START);
    String hash_tmp = {0};
    for (int i = 0; i < header->stringhash_count; ++i) {
        buffer->read_cstring(buffer,&hash_tmp);
        uint64 string_hash = 0;
        buffer->read_uint64(buffer, &string_hash);
        if (DA_contains(&lib->hash_strings.keys, &string_hash, compare_hashes64)) {
            continue;
        }
        String_copy_from(DM_insert(&lib->hash_strings, string_hash), &hash_tmp);
    }
    String_free(&hash_tmp);

    DA_init(&adf->strings, String, header->nametable_count);
    buffer->set_position(buffer, header->nametable_offset + header->nametable_count, BUFFER_ORIGIN_START);
    for (int i = 0; i < header->nametable_count; ++i) {
        buffer->read_cstring(buffer,DA_append_get(&adf->strings));
    }
    buffer->set_position(buffer, header->typedef_offset, BUFFER_ORIGIN_START);
    DA_init(&adf->type_defs, STI_TypeDef, header->typedef_count);
    for (int i = 0; i < header->typedef_count; ++i) {
        if (!read_typedef(adf, buffer, lib)) return false;
    }
    DA_init(&adf->instances, ADFInstance, header->instance_count);
    buffer->set_position(buffer, header->instance_offset, BUFFER_ORIGIN_START);
    for (int i = 0; i < header->instance_count; ++i){
        ADFInstance *instance = DA_append_get(&adf->instances);
        buffer->read(buffer, instance, sizeof(ADFInstance), NULL);
    }

    return true;
}


void ADF_free(ADF *adf) {
    String_free(&adf->comment);
    DA_free_with_inner(&adf->strings, {String_free(it);});
    DA_free(&adf->type_defs);
    DA_free(&adf->instances);
}

void ADF_load_builtin_adf(STI_TypeLibrary *lib, const uint8 *data, int64 size) {
        ADF adf = {0};
        MemoryBuffer emb = {0};
        MemoryBuffer_allocate(&emb, size);
        memcpy(emb.data, data, size);
        ADF_from_buffer(&adf, (Buffer*)&emb, lib);
        ADF_free(&adf);
        emb.close(&emb);
}
