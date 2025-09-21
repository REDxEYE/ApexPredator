// Created by RED on 19.09.2025.

#include "apex/adf/adf.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex/adf/adf_types.h"

bool _read_typedef(ADF *adf, Buffer *buffer, STI_TypeLibrary *lib) {
    STI_TypeDef *typedef_ = DA_append_get(&adf->type_defs);
    buffer->read(buffer, typedef_, sizeof(STI_TypeDef),NULL);
    STI_Type *type = STI_TypeLibrary_new_type(lib, typedef_->type, typedef_->type_hash);
    memcpy(&type->type_info, typedef_, sizeof(STI_TypeDef));

    String_copy_from(&type->name, &adf->strings.items[typedef_->name_id]);
    switch (typedef_->type) {
        case STI_Structure: {
            uint32 member_count = 0;
            if (buffer->read_uint32(buffer, &member_count)){
                return false;
            }
            DA_resize(&type->type_data.struct_data.members, member_count);
            for (int j = 0; j < member_count; ++j) {
                STI_StructMember *member = DA_at(&type->type_data.struct_data.members, j);
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
            DA_resize(&type->type_data.enum_data.members, member_count);
            for (int j = 0; j < member_count; ++j) {
                STI_EnumMember *member = DA_at(&type->type_data.enum_data.members, j);
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
    return true;
}

bool ADF_from_buffer(ADF *adf, Buffer *buffer) {
    ADFHeader *header = &adf->header;
    if (buffer->read(buffer, header, sizeof(ADFHeader),NULL) != BUFFER_SUCCESS) {
        return false;
    }
    buffer->read_cstring(buffer, &adf->comment);

    DA_init(&adf->strings, String, header->nametable_count);
    buffer->set_position(buffer, header->nametable_offset + header->nametable_count, BUFFER_ORIGIN_START);
    for (int i = 0; i < header->nametable_count; ++i) {
        buffer->read_cstring(buffer,DA_append_get(&adf->strings));
    }
    STI_TypeLibrary *lib = &adf->type_library;
    STI_TypeLibrary_init(lib);
    buffer->set_position(buffer, header->typedef_offset, BUFFER_ORIGIN_START);
    DA_init(&adf->type_defs, STI_TypeDef, header->typedef_count);
    for (int i = 0; i < header->typedef_count; ++i) {
        if (!_read_typedef(adf, buffer, lib)) return false;
    }
    DA_init(&adf->instances, ADFInstance, header->instance_count);
    buffer->set_position(buffer, header->instance_offset, BUFFER_ORIGIN_START);
    for (int i = 0; i < header->instance_count; ++i){
        ADFInstance *instance = DA_append_get(&adf->instances);
        buffer->read(buffer, instance, sizeof(ADFInstance), NULL);
    }

    return true;
}

void ADF_generate_readers(ADF *adf, String* namespace, FILE *output) {
    STI_start_type_dump(&adf->type_library);
    for (int32 i = (int32)adf->type_defs.count - 1; i >= 0; --i) {
        STI_TypeDef *type_def = DA_at(&adf->type_defs, i);
        STI_Type *type = DM_get(&adf->type_library.types, type_def->type_hash);
        STI_dump_type(&adf->type_library, type, output);
        fflush(output);
    }

    for (int32 i = (int32)adf->type_defs.count - 1; i >= 0; --i) {
        STI_TypeDef *type_def = DA_at(&adf->type_defs, i);
        STI_Type *type = DM_get(&adf->type_library.types, type_def->type_hash);
        STI_generate_reader_function(&adf->type_library, type, output, true);
    }
    fprintf(output, "\n\n");
    for (int32 i = (int32)adf->type_defs.count - 1; i >= 0; --i) {
        STI_TypeDef *type_def = DA_at(&adf->type_defs, i);
        STI_Type *type = DM_get(&adf->type_library.types, type_def->type_hash);
        STI_generate_reader_function(&adf->type_library, type, output, false);
    }
    STI_generate_register_function(&adf->type_library, namespace, output);
}
