// Created by RED on 19.09.2025.

#include "apex/adf/adf.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "apex/adf/adf_types.h"


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
    BufferError read_error;
    DA_init(&adf->type_defs, STI_TypeDef, header->typedef_count);
    for (int i = 0; i < header->typedef_count; ++i) {
        STI_TypeDef *typedef_ = DA_append_get(&adf->type_defs);
        buffer->read(buffer, typedef_, sizeof(STI_TypeDef),NULL);
        STI_Type *type = STI_TypeLibrary_new_type(lib, typedef_->type, typedef_->type_hash);
        memcpy(&type->type_info, typedef_, sizeof(STI_TypeDef));

        String_copy_from(&type->name, &adf->strings.items[typedef_->name_id]);
        switch (typedef_->type) {
            case STI_Structure: {
                uint32 member_count = buffer->read_uint32(buffer, &read_error);
                if (read_error != BUFFER_SUCCESS) {
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
                uint32 member_count = buffer->read_uint32(buffer, &read_error);
                if (read_error != BUFFER_SUCCESS) {
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
                type->type_data.array_data.count = buffer->read_uint32(buffer, &read_error);
                if (read_error != BUFFER_SUCCESS) {
                    return false;
                }
                break;
            }
            case STI_Bitfield:
            case STI_StringHash:
            case STI_Pointer: {
                type->type_data.unk_data.unk = buffer->read_uint32(buffer, &read_error);
                if (read_error != BUFFER_SUCCESS) {
                    return false;
                }
                break;
            }
            default: {
                printf("Unknown type %i\n", typedef_->type);
                assert(false && "Unknown type");
            };
        }
    }

    //s8 = 0x580D0A62
    STI_Type *type = STI_TypeLibrary_new_type(lib, STI_Primitive, 0x580D0A62);
    String_from_cstr(&type->name, "int8");
    type->type_info.type = STI_Primitive;
    type->type_info.size = 1;
    type->type_info.alignment = 1;
    type->type_info.type_hash = 0x580D0A62;
    type->type_info.name_id = UINT64_MAX;

    //u8 = 0x0ca2821d
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, 0x0CA2821D);
    String_from_cstr(&type->name, "uint8");
    type->type_info.type = STI_Primitive;
    type->type_info.size = 1;
    type->type_info.alignment = 1;
    type->type_info.type_hash = 0x0CA2821D;
    type->type_info.name_id = UINT64_MAX;

    //s16 = 0xD13FCF93
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, 0xD13FCF93);
    String_from_cstr(&type->name, "int16");
    type->type_info.type = STI_Primitive;
    type->type_info.size = 2;
    type->type_info.alignment = 2;
    type->type_info.type_hash = 0xD13FCF93;
    type->type_info.name_id = UINT64_MAX;

    //u16 = 0x86d152bd
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, 0x86D152BD);
    String_from_cstr(&type->name, "uint16");
    type->type_info.type = STI_Primitive;
    type->type_info.size = 2;
    type->type_info.alignment = 2;
    type->type_info.type_hash = 0x86D152BD;
    type->type_info.name_id = UINT64_MAX;

    //s32 = 0x192fe633
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, 0x192FE633);
    String_from_cstr(&type->name, "int32");
    type->type_info.type = STI_Primitive;
    type->type_info.size = 4;
    type->type_info.alignment = 4;
    type->type_info.type_hash = 0x192FE633;
    type->type_info.name_id = UINT64_MAX;

    //u32 = 0x075e4e4f
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, 0x075E4E4F);
    String_from_cstr(&type->name, "uint32");
    type->type_info.type = STI_Primitive;
    type->type_info.size = 4;
    type->type_info.alignment = 4;
    type->type_info.type_hash = 0x075E4E4F;
    type->type_info.name_id = UINT64_MAX;

    //s64 = 0xAF41354F
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, 0xAF41354F);
    String_from_cstr(&type->name, "int64");
    type->type_info.type = STI_Primitive;
    type->type_info.size = 8;
    type->type_info.alignment = 8;
    type->type_info.type_hash = 0xAF41354F;
    type->type_info.name_id = UINT64_MAX;

    //u64 = 0xA139E01F
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, 0xA139E01F);
    String_from_cstr(&type->name, "uint64");
    type->type_info.type = STI_Primitive;
    type->type_info.size = 8;
    type->type_info.alignment = 8;
    type->type_info.type_hash = 0xA139E01F;
    type->type_info.name_id = UINT64_MAX;

    //f32 = 0x7515A207
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, 0x7515A207);
    String_from_cstr(&type->name, "float32");
    type->type_info.type = STI_Primitive;
    type->type_info.size = 4;
    type->type_info.alignment = 4;
    type->type_info.type_hash = 0x7515A207;
    type->type_info.name_id = UINT64_MAX;

    //f64 = 0xC609F663
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, 0xC609F663);
    String_from_cstr(&type->name, "float64");
    type->type_info.type = STI_Primitive;
    type->type_info.size = 8;
    type->type_info.alignment = 8;
    type->type_info.type_hash = 0xC609F663;
    type->type_info.name_id = UINT64_MAX;

    //string = 0x8955583E
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, 0x8955583E);
    String_from_cstr(&type->name, "String");
    type->type_info.type = STI_Primitive;
    type->type_info.size = 16;
    type->type_info.alignment = 8;
    type->type_info.type_hash = 0x8955583E;
    type->type_info.name_id = UINT64_MAX;

    return true;
}
