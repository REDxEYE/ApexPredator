// Created by RED on 19.09.2025.

#include "apex/adf/sti.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

void STI_TypeLibrary_init(STI_TypeLibrary *lib) {
    DM_init(&lib->types, STI_Type, 64);
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
        if (type->type==STI_Bitfield ||
            type->type==STI_InlineArray ||
            type->type==STI_StringHash) {
            return;
        }

        fprintf(output, "bool read_%s(Buffer* buffer, %s* out);\n", String_data(&type_name), String_data(&type_name));
        String_free(&type_name);
        return;
    }
    switch (type->type) {
        case STI_Structure: {
            fprintf(output, "static bool read_%s(Buffer* buffer, %s* out) {\n", String_data(&type_name), String_data(&type_name));
            DynamicArray_STI_StructMember *members = &type->type_data.struct_data.members;
            for (int i = 0; i < members->count; ++i) {
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
                    case STI_Bitfield:
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
                    "    if (read_STI_uint32(buffer, &count) != BUFFER_SUCCESS) return false;\n"
                    "    DA_init(out, %s, count);\n"
                    "    for (uint32 i = 0; i < count; ++i) {\n"
                    "        if (!read_%s(buffer, &out->items[i])) return false;\n"
                    "    }\n"
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
                            "    if (read_STI_uint8(buffer, &value) != BUFFER_SUCCESS) return false;\n"
                            "    *out = (%s)value;\n"
                            "    return true;\n"
                            "}\n",
                            String_data(&type_name), String_data(&type_name), String_data(&type_name));
                    break;
                case 2:
                    fprintf(output,
                            "static bool read_%s(Buffer* buffer, %s* out) {\n"
                            "    uint16 value = 0;\n"
                            "    if (read_STI_uint16(buffer, &value) != BUFFER_SUCCESS) return false;\n"
                            "    *out = (%s)value;\n"
                            "    return true;\n"
                            "}\n",
                            String_data(&type_name), String_data(&type_name), String_data(&type_name));
                    break;
                case 4:
                    fprintf(output,
                            "static bool read_%s(Buffer* buffer, %s* out) {\n"
                            "    uint32 value = 0;\n"
                            "    if (read_STI_uint32(buffer, &value) != BUFFER_SUCCESS) return false;\n"
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
                STI_Type *member_type = DM_get(&lib->types, members.items[i].info.type_hash);
                String type_name = {0};
                STI_get_type_name(lib, member_type, &type_name);
                switch (member_type->type) {
                    case STI_InlineArray: {
                        fprintf(output, "    %s %s[%i];\n", String_data(&type_name),
                                String_data(&members.items[i].name), member_type->type_info.element_len);
                        break;
                    }
                    case STI_Bitfield: {
                        fprintf(output, "    %s %s:%i;\n", String_data(&type_name),
                                String_data(&members.items[i].name), member_type->type_info.element_len);
                        break;
                    }
                    default: {
                        fprintf(output, "    %s %s;\n", String_data(&type_name), String_data(&members.items[i].name));
                        break;
                    }
                }
                String_free(&type_name);
            }
            fprintf(output, "} %s;\n", String_data(&type->name));
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
