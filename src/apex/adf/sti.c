// Created by RED on 19.09.2025.

#include "apex/adf/sti.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

void STI_TypeLibrary_init(STI_TypeLibrary *lib) {
    DM_init(&lib->types, STI_Type, 64);
    DM_init(&lib->read_functions, read_type_fn, 64);

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
}

STI_Type *STI_TypeLibrary_new_type(STI_TypeLibrary *lib, STI_MetaType meta_type, uint32 type_hash) {
    STI_Type *type = DM_insert(&lib->types, type_hash);
    if (type) {
        type->type = meta_type;
        switch (meta_type) {
            case STI_Structure: {
                DA_init(&type->type_data.struct_data.members, STI_StructMember, 1);
                break;
            }
            case STI_Enumeration: {
                DA_init(&type->type_data.enum_data.members, STI_EnumMember, 1);
                break;
            }
            case STI_Primitive:
            case STI_Bitfield:
            case STI_Pointer:
            case STI_StringHash:
            case STI_Array:
            case STI_InlineArray: {
                break;
            }
            default: {
                printf("Unknown type %i\n", meta_type);
                assert(false && "Should not reach");
            };
        }
        return type;
    }
    return NULL;
}

uint32 STI_TypeLibrary_types_count(const STI_TypeLibrary *lib) {
    return DM_count(&lib->types);
}

void STI_start_type_dump(STI_TypeLibrary *lib) {
    DA_init(&lib->exported_hashes, uint32, 64);
}

void STI_get_type_name(STI_TypeLibrary *lib, STI_Type *type, String *type_name) {
    switch (type->type) {
        case STI_Structure:
        case STI_Enumeration: {
            String_copy_from(type_name, &type->name);
            break;
        }
        case STI_Primitive: {
            String_from_cstr(type_name, "STI_");
            String_append_str(type_name, &type->name);
            break;
        }
        case STI_Array: {
            STI_Type *inner_array_type = DM_get(&lib->types, type->type_info.element_type_hash);
            String tmp = {0};
            String_from_cstr(&tmp, "DynamicArray_");
            STI_get_type_name(lib, inner_array_type, type_name);
            String_append_str(&tmp, type_name);
            String_copy_from(type_name, &tmp);
            String_free(&tmp);
            break;
        }
        case STI_InlineArray: {
            STI_Type *inner_array_type = DM_get(&lib->types, type->type_info.element_type_hash);
            STI_get_type_name(lib, inner_array_type, type_name);
            break;
        }
        case STI_Bitfield: {
            String_copy_from(type_name, &type->name);
            int32 sep_id = String_find_chr(type_name, ':');
            assert(sep_id!=-1 && "Failed to find separator in bitfield.");
            String_resize(type_name, sep_id);
            break;
        }
        case STI_StringHash: {
            switch (type->type_info.size) {
                case 4: {
                    String_from_cstr(type_name, "uint32");
                    break;
                }
                default: {
                    printf("Unknown string hash size %i\n", type->type);
                    assert(false && "Unknown string hash size");
                }
            }

            break;
        }
        default: {
            printf("Unknown type %i\n", type->type);
            assert(false && "Unknown type");
        };
    }
}

void STI_dump_primitives(STI_TypeLibrary *lib, FILE *output) {
    for (int i = 0; i < STI_TypeLibrary_types_count(lib); ++i) {
        STI_Type *type = DM_get_value(&lib->types, i);
        if (type->type == STI_Primitive) {
            char *type_name_cstr = String_data(&type->name);
            fprintf(output, "typedef %s STI_%s;\n", type_name_cstr, type_name_cstr);
        }
    }
    fprintf(output, "\n");
}

void STI_generate_reader_function(STI_TypeLibrary *lib, STI_Type *type, FILE *output, bool prototype_only) {
    if (type->type == STI_Primitive) {
        return;
    }

    String type_name = {};
    STI_get_type_name(lib, type, &type_name);

    if (prototype_only) {
        if (type->type == STI_Bitfield ||
            type->type == STI_InlineArray ||
            type->type == STI_StringHash) {
            return;
        }

        fprintf(output, "bool read_%s(Buffer* buffer, %s* out);\n", String_data(&type_name), String_data(&type_name));
        String_free(&type_name);
        return;
    }
    switch (type->type) {
        case STI_Structure: {
            fprintf(output, "static bool read_%s(Buffer* buffer, %s* out) {\n", String_data(&type_name),
                    String_data(&type_name));
            DynamicArray_STI_StructMember *members = &type->type_data.struct_data.members;
            for (uint32 i = 0; i < members->count; ++i) {
                STI_StructMember *member = &members->items[i];
                STI_Type *member_type = DM_get(&lib->types, member->info.type_hash);
                String member_type_name = {0};
                STI_get_type_name(lib, member_type, &member_type_name);
                switch (member_type->type) {
                    case STI_InlineArray: {
                        fprintf(output,
                                "    for (uint32 i = 0; i < %i; ++i) {\n"
                                "        if (!read_%s(buffer, &out->%s[i])) return false;\n"
                                "    }\n",
                                member_type->type_info.element_len, String_data(&member_type_name),
                                String_data(&member->name));
                        break;
                    }
                    case STI_Array: {
                        fprintf(output,
                                "    if (!read_%s(buffer, &out->%s)) return false;\n",
                                String_data(&member_type_name), String_data(&member->name));
                        break;
                    }
                    case STI_Bitfield: {
                        uint32 first_bit_member = i;
                        uint32 last_bit_member = i;
                        uint32 bits_used = 0;
                        for (uint32 j = i; j < members->count; ++j) {
                            STI_StructMember *bit_member = &members->items[j];
                            STI_Type *bit_member_type = DM_get(&lib->types, bit_member->info.type_hash);
                            if (bit_member_type->type!=STI_Bitfield) {
                                last_bit_member = j-1;
                                break;
                            }
                            bits_used+=bit_member_type->type_info.element_len;
                            printf("");
                        }
                        if (bits_used<=8) {
                            fprintf(output,
                                    "    uint8 bitfield_value = 0;\n"
                                    "    if (!read_STI_uint8(buffer, &bitfield_value)) return false;\n");
                        } else if (bits_used<=16) {
                            fprintf(output,
                                    "    uint16 bitfield_value = 0;\n"
                                    "    if (!read_STI_uint16(buffer, &bitfield_value)) return false;\n");
                        } else if (bits_used<=32) {
                            fprintf(output,
                                    "    uint32 bitfield_value = 0;\n"
                                    "    if (!read_STI_uint32(buffer, &bitfield_value)) return false;\n");
                        } else if (bits_used<=64) {
                            fprintf(output,
                                    "    uint64 bitfield_value = 0;\n"
                                    "    if (!read_STI_uint64(buffer, &bitfield_value)) return false;\n");
                        } else {
                            assert(false && "Bitfield too large");
                        }
                        uint32 bit_offset = 0;
                        for (uint32 j = first_bit_member; j <= last_bit_member; ++j)
                        {
                            STI_StructMember *bit_member = &members->items[j];
                            STI_Type *bit_member_type = DM_get(&lib->types, bit_member->info.type_hash);
                            fprintf(output,
                                    "    out->%s = (bitfield_value >> %i) & 0x%X;\n",
                                    String_data(&bit_member->name), bit_offset,
                                    (1u << bit_member_type->type_info.element_len) - 1);
                            bit_offset += bit_member_type->type_info.element_len;
                            i = j;
                        }
                        i = last_bit_member;
                        break;
                    }
                    case STI_StringHash:
                    case STI_Pointer: {
                        // Skip
                        break;
                    }
                    default: {
                        fprintf(output,
                                "    if (!read_%s(buffer, &out->%s)) return false;\n",
                                String_data(&member_type_name), String_data(&member->name));
                        break;
                    }
                }
                String_free(&member_type_name);
            }
            fprintf(output, "    return true;\n}\n");
            break;
        }
        case STI_Array: {
            STI_Type *inner_array_type = DM_get(&lib->types, type->type_info.element_type_hash);
            String inner_array_type_name = {0};
            STI_get_type_name(lib, inner_array_type, &inner_array_type_name);
            char *type_name_cstr = String_data(&type_name);
            char *inner_type_name_cstr = String_data(&inner_array_type_name);
            fprintf(output,
                    "static bool read_%s(Buffer* buffer, %s* out) {\n"
                    "    uint32 count = 0;\n"
                    "    uint32 offset = 0;\n"
                    "    if (!read_STI_uint32(buffer, &offset)) return false;\n"
                    "    if (!read_STI_uint32(buffer, &count)) return false;\n"
                    "    DA_init(out, %s, count);\n"
                    "    int64 original_offset = 0;\n"
                    "    if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;\n"
                    "    for (uint32 i = 0; i < count; ++i) {\n"
                    "        if (!read_%s(buffer, &out->items[i])) return false;\n"
                    "    }\n"
                    "    if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;\n"
                    "    return true;\n"
                    "}\n",
                    type_name_cstr, type_name_cstr, inner_type_name_cstr, inner_type_name_cstr);
            String_free(&inner_array_type_name);
            break;
        }
        case STI_Enumeration: {
            switch (type->type_info.size) {
                case 1:
                    fprintf(output,
                            "static bool read_%s(Buffer* buffer, %s* out) {\n"
                            "    uint8 value = 0;\n"
                            "    if (!read_STI_uint8(buffer, &value)) return false;\n"
                            "    *out = (%s)value;\n"
                            "    return true;\n"
                            "}\n",
                            String_data(&type_name), String_data(&type_name), String_data(&type_name));
                    break;
                case 2:
                    fprintf(output,
                            "static bool read_%s(Buffer* buffer, %s* out) {\n"
                            "    uint16 value = 0;\n"
                            "    if (!read_STI_uint16(buffer, &value)) return false;\n"
                            "    *out = (%s)value;\n"
                            "    return true;\n"
                            "}\n",
                            String_data(&type_name), String_data(&type_name), String_data(&type_name));
                    break;
                case 4:
                    fprintf(output,
                            "static bool read_%s(Buffer* buffer, %s* out) {\n"
                            "    uint32 value = 0;\n"
                            "    if (!read_STI_uint32(buffer, &value)) return false;\n"
                            "    *out = (%s)value;\n"
                            "    return true;\n"
                            "}\n",
                            String_data(&type_name), String_data(&type_name), String_data(&type_name));
                    break;
                default: {
                    printf("Unknown enum size %i\n", type->type_info.size);
                    assert(false && "Unknown enum size");
                }
            }
        }

        case STI_InlineArray:
        case STI_Bitfield:
        case STI_StringHash:
            return;
        default: {
            printf("Unknown type %i\n", type->type);
            assert(false && "Unknown type");
        }
    }
}

void STI_generate_register_function(STI_TypeLibrary *lib, String *namespace, FILE *output) {
    fprintf(output, "static void STI_%s_register_functions(STI_TypeLibrary* lib) {\n    void** slot;\n",
            String_data(namespace));
    String type_name = {0};
    for (int i = 0; i < DM_count(&lib->types); ++i) {
        STI_Type *type = DM_get_value(&lib->types, i);
        if (type->type == STI_Primitive ||
            type->type == STI_Bitfield ||
            type->type == STI_InlineArray ||
            type->type == STI_StringHash) {
            continue;
        }
        STI_get_type_name(lib, type, &type_name);
        fprintf(output, "    slot = DM_insert(&lib->read_functions, 0x%08X); ",
                type->type_info.type_hash);
        fprintf(output, "    *slot = read_%s;\n", String_data(&type_name));
    }
    fprintf(output, "}\n");
    String_free(&type_name);
}

bool compare_hashes(const uint32 *a, const uint32 *b) {
    return *a == *b;
}

void STI_dump_type(STI_TypeLibrary *lib, STI_Type *type, FILE *output) {
    if (DA_contains(&lib->exported_hashes, &type->type_info.type_hash, compare_hashes)) {
        return;
    }
    switch (type->type) {
        case STI_Structure: {
            DynamicArray_STI_StructMember members = type->type_data.struct_data.members;
            for (int i = 0; i < members.count; ++i) {
                STI_dump_type(lib, DM_get(&lib->types, members.items[i].info.type_hash), output);
            }

            fprintf(output, "typedef struct{\n");
            for (int i = 0; i < members.count; ++i) {
                STI_StructMember *member = &members.items[i];
                STI_Type *member_type = DM_get(&lib->types, member->info.type_hash);
                String type_name = {0};
                STI_get_type_name(lib, member_type, &type_name);
                switch (member_type->type) {
                    case STI_InlineArray: {
                        fprintf(output, "    %s %s[%i];", String_data(&type_name),
                                String_data(&member->name), member_type->type_info.element_len);
                        break;
                    }
                    case STI_Bitfield: {
                        fprintf(output, "    %s %s:%i;", String_data(&type_name),
                                String_data(&member->name), member_type->type_info.element_len);
                        break;
                    }
                    default: {
                        fprintf(output, "    %s %s;", String_data(&type_name), String_data(&member->name));
                        break;
                    }
                }
                fprintf(output, " // offset: %i, size: %i\n", member->info.offset, member->info.size);
                String_free(&type_name);
            }
            fprintf(output, "} %s; // size: %i\n", String_data(&type->name), type->type_info.size);
            break;
        }
        case STI_Enumeration: {
            fprintf(output, "typedef enum{ // size: %i\n", type->type_info.size);
            DynamicArray_STI_EnumMember members = type->type_data.enum_data.members;
            for (int i = 0; i < members.count; ++i) {
                fprintf(output, "    %s = %u,\n", String_data(&members.items[i].name), members.items[i].info.value);
            }
            fprintf(output, "} %s;\n", String_data(&type->name));
            break;
        }
        case STI_Array: {
            String tmp = {0};
            STI_Type *inner_array_type = DM_get(&lib->types, type->type_info.element_type_hash);
            STI_dump_type(lib, inner_array_type, output);
            STI_get_type_name(lib, inner_array_type, &tmp);
            fprintf(output, "DYNAMIC_ARRAY_STRUCT(%s, %s);\n", String_data(&tmp), String_data(&tmp));
            String_free(&tmp);
            break;
        }
        case STI_Primitive: {
            // char *type_name_cstr = String_data(&type->name);
            // fprintf(output, "typedef %s STI_%s;\n", type_name_cstr, type_name_cstr);
            break;
        }
        case STI_InlineArray: {
            STI_Type *inner_array_type = DM_get(&lib->types, type->type_info.element_type_hash);
            STI_dump_type(lib, inner_array_type, output);
            break;
        }

        case STI_StringHash:
        case STI_Bitfield:
        case STI_Pointer: {
            return;
        }
        default: {
            printf("Unknown type %i\n", type->type);
            assert(false && "Unknown type");
        }
    }
    DA_append(&lib->exported_hashes, &type->type_info.type_hash);
    fprintf(output, "\n");
}
