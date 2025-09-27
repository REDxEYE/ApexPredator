// Created by RED on 19.09.2025.

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils/common.h"
#include "utils/lookup3.h"
#include "apex/adf/sti.h"


void STI_TypeLibrary_init(STI_TypeLibrary *lib) {
    DM_init(&lib->name_hash_to_type, uint32, 64);
    DM_init(&lib->types, STI_Type, 64);
    DM_init(&lib->object_functions, STI_ObjectMethods, 64);
    DM_init(&lib->hash_strings, String, 64);

    String tmp = {0};

    //s8 = 0x580D0A62
    STI_Type *type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_INT8,
                                              String_move(String_from_cstr(&tmp, "int8")));
    type->info.type = STI_Primitive;
    type->info.size = 1;
    type->info.alignment = 1;
    type->info.hash = STI_TYPE_HASH_INT8;
    type->info.name_id = UINT64_MAX;

    //u8 = 0x0ca2821d
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_UINT8,
                                    String_move(String_from_cstr(&tmp, "uint8")));
    type->info.type = STI_Primitive;
    type->info.size = 1;
    type->info.alignment = 1;
    type->info.hash = STI_TYPE_HASH_UINT8;
    type->info.name_id = UINT64_MAX;

    //s16 = 0xD13FCF93
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_INT16,
                                    String_move(String_from_cstr(&tmp, "int16")));
    type->info.type = STI_Primitive;
    type->info.size = 2;
    type->info.alignment = 2;
    type->info.hash = STI_TYPE_HASH_INT16;
    type->info.name_id = UINT64_MAX;

    //u16 = 0x86d152bd
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_UINT16,
                                    String_move(String_from_cstr(&tmp, "uint16")));
    type->info.type = STI_Primitive;
    type->info.size = 2;
    type->info.alignment = 2;
    type->info.hash = STI_TYPE_HASH_UINT16;
    type->info.name_id = UINT64_MAX;

    //s32 = 0x192fe633
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_INT32,
                                    String_move(String_from_cstr(&tmp, "int32")));
    type->info.type = STI_Primitive;
    type->info.size = 4;
    type->info.alignment = 4;
    type->info.hash = STI_TYPE_HASH_INT32;
    type->info.name_id = UINT64_MAX;

    //u32 = 0x075e4e4f
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_UINT32,
                                    String_move(String_from_cstr(&tmp, "uint32")));
    type->info.type = STI_Primitive;
    type->info.size = 4;
    type->info.alignment = 4;
    type->info.hash = STI_TYPE_HASH_UINT32;
    type->info.name_id = UINT64_MAX;

    //s64 = 0xAF41354F
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_INT64,
                                    String_move(String_from_cstr(&tmp, "int64")));
    type->info.type = STI_Primitive;
    type->info.size = 8;
    type->info.alignment = 8;
    type->info.hash = STI_TYPE_HASH_INT64;
    type->info.name_id = UINT64_MAX;

    //u64 = 0xA139E01F
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_UINT64,
                                    String_move(String_from_cstr(&tmp, "uint64")));
    type->info.type = STI_Primitive;
    type->info.size = 8;
    type->info.alignment = 8;
    type->info.hash = STI_TYPE_HASH_UINT64;
    type->info.name_id = UINT64_MAX;

    //f32 = 0x7515A207
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_FLOAT32,
                                    String_move(String_from_cstr(&tmp, "float32")));
    type->info.type = STI_Primitive;
    type->info.size = 4;
    type->info.alignment = 4;
    type->info.hash = STI_TYPE_HASH_FLOAT32;
    type->info.name_id = UINT64_MAX;

    //f64 = 0xC609F663
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_FLOAT64,
                                    String_move(String_from_cstr(&tmp, "float64")));
    type->info.type = STI_Primitive;
    type->info.size = 8;
    type->info.alignment = 8;
    type->info.hash = STI_TYPE_HASH_FLOAT64;
    type->info.name_id = UINT64_MAX;

    //string = 0x8955583E
    type = STI_TypeLibrary_new_type(lib, STI_Primitive, STI_TYPE_HASH_STRING,
                                    String_move(String_from_cstr(&tmp, "String")));
    type->info.type = STI_Primitive;
    type->info.size = 16;
    type->info.alignment = 8;
    type->info.hash = STI_TYPE_HASH_STRING;
    type->info.name_id = UINT64_MAX;

    //UnkSpecial = 0xDEFE88ED
    type = STI_TypeLibrary_new_type(lib, STI_DeferredType, STI_TYPE_HASH_UNK,
                                    String_move(String_from_cstr(&tmp, "Deferred")));
    type->info.type = STI_DeferredType;
    type->info.size = 16;
    type->info.alignment = 8;
    type->info.hash = STI_TYPE_HASH_UNK;
    type->info.name_id = UINT64_MAX;

    String_free(&tmp);
}

STI_Type *STI_TypeLibrary_new_type(STI_TypeLibrary *lib, STI_MetaType meta_type, uint32 type_hash, String *name) {
    STI_Type *type = DM_insert(&lib->types, type_hash);
    uint32 *slot = NULL;
    if (type) {
        STI_Type_init(type, meta_type);
        String_copy_from(&type->name, name);
        uint32 name_hash = 0;
        uint32 init_tmp = 0;
        hashlittle2(String_data(&type->name), type->name.size, &name_hash, &init_tmp);

        uint32 *other = DM_get(&lib->name_hash_to_type, name_hash);

        if (other != NULL) {
            if ((*other) == type_hash || meta_type == STI_InlineArray) {
                //same type, no problem
                return type;
            }

            printf("Warning: Name collision for type name %s (hash %08X)\n", String_data(&type->name), name_hash);
            String_append_cstr(&type->name, "_2");
            init_tmp = 0;
            hashlittle2(String_data(&type->name), type->name.size, &name_hash, &init_tmp);
            if (DM_get(&lib->name_hash_to_type, name_hash) != NULL) {
                printf("Error: Second name collision for type name %s (hash %08X)\n", String_data(&type->name),
                       name_hash);
                exit(1);
            }
            slot = DM_insert(&lib->name_hash_to_type, name_hash);
        } else {
            slot = DM_insert(&lib->name_hash_to_type, name_hash);
        }
        *slot = type_hash;

        return type;
    }
    return NULL;
}

int32 STI_TypeLibrary_types_count(const STI_TypeLibrary *lib) {
    return DM_count(&lib->types);
}

void STI_start_type_dump(STI_TypeLibrary *lib) {
    DA_init(&lib->exported_hashes, uint32, 64);
    *(uint32 *) (DA_append_get(&lib->exported_hashes)) = STI_TYPE_HASH_INT8;
    *(uint32 *) (DA_append_get(&lib->exported_hashes)) = STI_TYPE_HASH_UINT8;
    *(uint32 *) (DA_append_get(&lib->exported_hashes)) = STI_TYPE_HASH_INT16;
    *(uint32 *) (DA_append_get(&lib->exported_hashes)) = STI_TYPE_HASH_UINT16;
    *(uint32 *) (DA_append_get(&lib->exported_hashes)) = STI_TYPE_HASH_INT32;
    *(uint32 *) (DA_append_get(&lib->exported_hashes)) = STI_TYPE_HASH_UINT32;
    *(uint32 *) (DA_append_get(&lib->exported_hashes)) = STI_TYPE_HASH_INT64;
    *(uint32 *) (DA_append_get(&lib->exported_hashes)) = STI_TYPE_HASH_UINT64;
    *(uint32 *) (DA_append_get(&lib->exported_hashes)) = STI_TYPE_HASH_FLOAT32;
    *(uint32 *) (DA_append_get(&lib->exported_hashes)) = STI_TYPE_HASH_FLOAT64;
    *(uint32 *) (DA_append_get(&lib->exported_hashes)) = STI_TYPE_HASH_STRING;
}

void STI_get_type_name(STI_TypeLibrary *lib, STI_Type *type, String *type_name) {
    switch (type->type) {
        case STI_Structure:
        case STI_Enumeration: {
            String_copy_from(type_name, &type->name);
            break;
        }
        case STI_DeferredType:
        case STI_Primitive: {
            String_from_cstr(type_name, "STI_");
            String_append_str(type_name, &type->name);
            break;
        }
        case STI_Pointer: {
            String_from_cstr(type_name, "STI_");
            String_append_str(type_name, &type->name);
            break;
        }
        case STI_Array: {
            STI_Type *inner_array_type = DM_get(&lib->types, type->info.element_type_hash);
            String tmp = {0};
            String_from_cstr(&tmp, "DynamicArray_");
            STI_get_type_name(lib, inner_array_type, type_name);
            String_append_str(&tmp, type_name);
            String_copy_from(type_name, String_move(&tmp));
            String_free(&tmp);
            break;
        }
        case STI_InlineArray: {
            STI_Type *inner_array_type = DM_get(&lib->types, type->info.element_type_hash);
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
            switch (type->info.hash) {
                case 0xc03f64bf: {
                    String_from_cstr(type_name, "StringHash_48c5294d_4");
                    break;
                }
                case 0x7421fad9: {
                    String_from_cstr(type_name, "StringHash_99cfa095_6");
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

void STI_generate_reader_function(STI_TypeLibrary *lib, STI_Type *type, FILE *output, bool prototype_only) {
    if (type->type == STI_Primitive || type->type == STI_Pointer || type->type == STI_DeferredType) {
        return;
    }

    if (prototype_only) {
        if (type->type == STI_Bitfield ||
            type->type == STI_InlineArray ||
            type->type == STI_StringHash) {
            return;
        }
        String type_name = {};
        STI_get_type_name(lib, type, &type_name);
        fprintf(output, "static bool read_%s(Buffer* buffer, STI_TypeLibrary* lib, %s* out);\n",
                String_data(&type_name),
                String_data(&type_name));
        String_free(&type_name);
        return;
    }

    String type_name = {};
    STI_get_type_name(lib, type, &type_name);
    const char *type_name_cstr = String_data(&type_name);
    switch (type->type) {
        case STI_Structure: {
            fprintf(output, "static bool read_%s(Buffer* buffer, STI_TypeLibrary* lib, %s* out) {\n", type_name_cstr,
                    type_name_cstr);
            DynamicArray_STI_StructMember *members = &type->type_data.struct_data.members;
            uint32 running_offset = 0;
            String member_type_name = {0};
            for (uint32 i = 0; i < members->count; ++i) {
                STI_StructMember *member = &members->items[i];
                STI_Type *member_type = DM_get(&lib->types, member->info.type_hash);
                STI_get_type_name(lib, member_type, &member_type_name);
                if (running_offset != member->info.offset) {
                    uint32 pad_size = member->info.offset - running_offset;
                    fprintf(output, "    buffer->skip(buffer, %i);\n", pad_size);
                    running_offset += pad_size;
                }
                switch (member_type->type) {
                    case STI_InlineArray: {
                        fprintf(output,
                                "    for (uint32 i = 0; i < %i; ++i) {\n"
                                "        if (!read_%s(buffer, lib, &out->%s[i])) return false;\n"
                                "    }\n",
                                member_type->info.element_len, String_data(&member_type_name),
                                String_data(&member->name));
                        running_offset += member_type->info.size;
                        break;
                    }
                    case STI_Array: {
                        fprintf(output,
                                "    if (!read_%s(buffer, lib, &out->%s)) return false;\n",
                                String_data(&member_type_name), String_data(&member->name));
                        running_offset += 16;
                        break;
                    }
                    case STI_Bitfield: {
                        fprintf(output, "{\n");
                        uint32 first_bit_member = i;
                        uint32 last_bit_member = i;
                        STI_Type *total_type = DM_get(&lib->types, members->items[i].info.type_hash);
                        for (uint32 j = i; j < members->count; j++) {
                            STI_StructMember *bit_member = &members->items[j];
                            STI_Type *bit_member_type = DM_get(&lib->types, bit_member->info.type_hash);
                            if (bit_member_type->type == STI_Bitfield) {
                                last_bit_member = j;
                            } else {
                                last_bit_member = j - 1;
                                break;
                            }
                        }
                        String bit_type_name = {0};
                        STI_get_type_name(lib, total_type, &bit_type_name);
                        fprintf(output,
                                "        %s bitfield_value = 0;\n"
                                "        if (!read_STI_%s(buffer, lib, &bitfield_value)) return false;\n",
                                String_data(&bit_type_name),
                                String_data(&bit_type_name));
                        String_free(&bit_type_name);
                        uint32 bit_offset = 0;
                        for (uint32 j = first_bit_member; j <= last_bit_member; ++j) {
                            STI_StructMember *bit_member = &members->items[j];
                            STI_Type *bit_member_type = DM_get(&lib->types, bit_member->info.type_hash);
                            fprintf(output,
                                    "        out->%s = (bitfield_value >> %i) & 0x%X;\n",
                                    String_data(&bit_member->name), bit_offset,
                                    (1u << bit_member_type->info.element_len) - 1);
                            bit_offset += bit_member_type->info.element_len;
                        }
                        running_offset += total_type->info.size;
                        i = last_bit_member;
                        fprintf(output, "}\n");
                        break;
                    }
                    case STI_StringHash: {
                        fprintf(output,
                                "    if (!read_%s(buffer, lib, &out->%s)) return false;\n",
                                String_data(&member_type_name), String_data(&member->name));
                        running_offset += member_type->info.size;
                        break;
                    }
                    case STI_Pointer: {
                        // assert(false && "Pointers are not supported yet");
                        // fprintf(output, "buffer->skip(buffer, 4);\n // Unknown how pointers work");
                        fprintf(output,
                                "    assert(false && \"Pointers are not supported yet\");\n // Unknown how pointers work\n");
                        break;
                    }
                    default: {
                        fprintf(output,
                                "    if (!read_%s(buffer, lib, &out->%s)) return false;\n",
                                String_data(&member_type_name), String_data(&member->name));
                        running_offset += member_type->info.size;
                        break;
                    }
                }
            }
            if (running_offset != type->info.size) {
                fprintf(output, "    buffer->skip(buffer, %i);\n", type->info.size - running_offset);
            }
            fprintf(output, "    return true;\n}\n");
            String_free(&member_type_name);
            break;
        }
        case STI_Array: {
            STI_Type *inner_array_type = DM_get(&lib->types, type->info.element_type_hash);
            String inner_array_type_name = {0};
            STI_get_type_name(lib, inner_array_type, &inner_array_type_name);
            char *inner_type_name_cstr = String_data(&inner_array_type_name);
            fprintf(output,
                    "static bool read_%s(Buffer* buffer, STI_TypeLibrary* lib, %s* out) {\n"
                    "    uint32 count = 0;\n"
                    "    uint32 unk0 = 0;\n"
                    "    uint32 offset = 0;\n"
                    "    uint32 unk1 = 0;\n"
                    "    if (!read_STI_uint32(buffer, lib, &offset)) return false;\n"
                    "    if (!read_STI_uint32(buffer, lib, &unk0)) return false;\n"
                    "    if (!read_STI_uint32(buffer, lib, &count)) return false;\n"
                    "    if (!read_STI_uint32(buffer, lib, &unk1)) return false;\n"
                    "    DA_init(out, %s, count);\n"
                    "    if (count>0) {\n"
                    "        int64 original_offset = 0;\n"
                    "        if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;\n"
                    "        if(buffer->set_position(buffer, offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;\n"
                    "        for (uint32 i = 0; i < count; ++i) {\n"
                    "            if (!read_%s(buffer, lib, &out->items[i])) return false;\n"
                    "        }\n"
                    "        out->count = count;\n"
                    "        if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;\n"
                    "    }\n"
                    "    return true;\n"
                    "}\n",
                    type_name_cstr, type_name_cstr, inner_type_name_cstr, inner_type_name_cstr);
            String_free(&inner_array_type_name);
            break;
        }
        case STI_Enumeration: {
            switch (type->info.size) {
                case 1:
                    fprintf(output,
                            "static bool read_%s(Buffer* buffer, STI_TypeLibrary* lib, %s* out) {\n"
                            "    uint8 value = 0;\n"
                            "    if (!read_STI_uint8(buffer, lib, &value)) return false;\n"
                            "    *out = (%s)value;\n"
                            "    return true;\n"
                            "}\n",
                            type_name_cstr, type_name_cstr, type_name_cstr);
                    break;
                case 2:
                    fprintf(output,
                            "static bool read_%s(Buffer* buffer, STI_TypeLibrary* lib, %s* out) {\n"
                            "    uint16 value = 0;\n"
                            "    if (!read_STI_uint16(buffer, lib, &value)) return false;\n"
                            "    *out = (%s)value;\n"
                            "    return true;\n"
                            "}\n",
                            type_name_cstr, type_name_cstr, type_name_cstr);
                    break;
                case 4:
                    fprintf(output,
                            "static bool read_%s(Buffer* buffer, STI_TypeLibrary* lib, %s* out) {\n"
                            "    uint32 value = 0;\n"
                            "    if (!read_STI_uint32(buffer, lib, &value)) return false;\n"
                            "    *out = (%s)value;\n"
                            "    return true;\n"
                            "}\n",
                            type_name_cstr, type_name_cstr, type_name_cstr);
                    break;
                default: {
                    printf("Unknown enum size %i\n", type->info.size);
                    assert(false && "Unknown enum size");
                }
            }
        }
        case STI_InlineArray:
        case STI_Bitfield:
        case STI_StringHash:
        case STI_Pointer:
            break;
        default: {
            printf("Unknown type %i\n", type->type);
            assert(false && "Unknown type");
        }
    }
    String_free(&type_name);
}

void STI_generate_free_function(STI_TypeLibrary *lib, STI_Type *type, FILE *output, bool prototype_only) {
    if (type->type == STI_Bitfield ||
        type->type == STI_DeferredType ||
        type->type == STI_InlineArray ||
        type->type == STI_Primitive ||
        type->type == STI_Pointer ||
        type->type == STI_StringHash) {
        return;
    }

    String type_name = {};
    STI_get_type_name(lib, type, &type_name);

    char *type_name_cstr = String_data(&type_name);
    if (prototype_only) {
        fprintf(output, "static void free_%s(%s* obj, STI_TypeLibrary* lib);\n", type_name_cstr, type_name_cstr);
        String_free(&type_name);
        return;
    }
    fprintf(output, "static void free_%s(%s* obj, STI_TypeLibrary* lib) {\n", type_name_cstr, type_name_cstr);
    switch (type->type) {
        case STI_Structure: {
            DynamicArray_STI_StructMember *members = &type->type_data.struct_data.members;
            String member_type_name = {0};
            for (uint32 i = 0; i < members->count; ++i) {
                STI_StructMember *member = &members->items[i];
                STI_Type *member_type = DM_get(&lib->types, member->info.type_hash);
                STI_get_type_name(lib, member_type, &member_type_name);
                switch (member_type->type) {
                    case STI_InlineArray: {
                        // Inline arrays do need innter items freed
                        STI_Type *inner_type = DM_get(&lib->types, member_type->info.element_type_hash);
                        if (inner_type->type != STI_Primitive && inner_type->type != STI_StringHash) {
                            // No need to free primitives or string hashes
                            fprintf(output,
                                    "    for (uint32 i = 0; i < %i; ++i) {\n"
                                    "        free_%s(&obj->%s[i], lib);\n"
                                    "    }\n",
                                    member_type->info.element_len, String_data(&member_type_name),
                                    String_data(&member->name));
                        }
                        break;
                    }
                    case STI_Array: {
                        fprintf(output,
                                "    free_%s(&obj->%s, lib);\n",
                                String_data(&member_type_name), String_data(&member->name));
                        break;
                    }
                    case STI_Bitfield: {
                        // Bitfields do not need to be freed
                        break;
                    }
                    case STI_StringHash: {
                        // String hashes do not need to be freed
                        break;
                    }
                    case STI_Primitive: {
                        // String hashes do not need to be freed
                        break;
                    }
                    case STI_Pointer: {
                        // assert(false && "Pointers are not supported yet");
                        fprintf(output,
                                "    assert(false && \"Pointers are not supported yet\");\n // Unknown how pointers work\n");
                        // Skip
                        break;
                    }
                    default: {
                        fprintf(output,
                                "    free_%s(&obj->%s, lib);\n",
                                String_data(&member_type_name), String_data(&member->name));
                        break;
                    }
                }
            }
            String_free(&member_type_name);
            break;
        }
        case STI_Array: {
            STI_Type *inner_type = DM_get(&lib->types, type->info.element_type_hash);
            String inner_type_name = {};
            STI_get_type_name(lib, inner_type, &inner_type_name);
            if ((inner_type->type != STI_Primitive && inner_type->type != STI_StringHash) || inner_type->info.
                hash == STI_TYPE_HASH_STRING) {
                fprintf(output,
                        "    for (uint32 i = 0; i < obj->count; ++i) {\n"
                        "        free_%s(&obj->items[i], lib);\n"
                        "    }\n",
                        String_data(&inner_type_name));
            }
            fprintf(output,
                    "    DA_free(obj);\n");
            String_free(&inner_type_name);
            break;
        }
        case STI_Enumeration:
        case STI_Primitive:
        case STI_DeferredType:
            // Nothing to free
            break;
        case STI_InlineArray:
        case STI_Bitfield:
        case STI_StringHash:
        case STI_Pointer:
            break;
        default: {
            printf("Unknown type %i\n", type->type);
            assert(false && "Unknown type");
        }
    }
    fprintf(output, "}\n");
    String_free(&type_name);
}

void STI_generate_print_function(STI_TypeLibrary *lib, STI_Type *type, FILE *output, bool prototype_only) {
    if (type->type == STI_Bitfield ||
        type->type == STI_InlineArray ||
        type->type == STI_DeferredType ||
        type->type == STI_Primitive ||
        type->type == STI_Pointer ||
        type->type == STI_StringHash) {
        return;
    }

    String type_name = {};
    STI_get_type_name(lib, type, &type_name);
    if (prototype_only) {
        fprintf(output, "static void print_%s(const %s* obj, STI_TypeLibrary* lib, FILE* handle, uint32 indent);\n",
                String_data(&type_name),
                String_data(&type_name));
        String_free(&type_name);
        return;
    }

    fprintf(output, "static void print_%s(const %s* obj, STI_TypeLibrary* lib, FILE* handle, uint32 indent) {\n",
            String_data(&type_name),
            String_data(&type_name));
    fprintf(output, "    fprintf(handle, \"%s {\\n\");\n    indent++;\n", String_data(&type_name));
    switch (type->type) {
        case STI_Structure: {
            DynamicArray_STI_StructMember *members = &type->type_data.struct_data.members;
            String member_type_name = {0};
            for (uint32 i = 0; i < members->count; ++i) {
                STI_StructMember *member = &members->items[i];
                STI_Type *member_type = DM_get(&lib->types, member->info.type_hash);
                STI_get_type_name(lib, member_type, &member_type_name);
                char *member_name_cstr = String_data(&member->name);
                fprintf(output, "    fprintf(handle, \"%%*s%%s = \", indent * 4, \"\", \"%s\");\n", member_name_cstr);
                switch (member_type->type) {
                    case STI_InlineArray: {
                        fprintf(output,
                                "    fprintf(handle, \"{\\n%%*s\", (indent+1)*4, \"\");\n"
                                "    for (uint32 i = 0; i < %i; ++i) {\n"
                                "        print_%s(&obj->%s[i], lib, handle, indent+1);\n"
                                "        if(i < %i){\n"
                                "            fprintf(handle,\", \\n%%*s\", (indent+1)*4, \"\");\n"
                                "        }\n"
                                "    }\n"
                                "    fprintf(handle, \"\\n%%*s}\", (indent+1)*4, \"\");\n",
                                member_type->info.element_len, String_data(&member_type_name),
                                member_name_cstr, member_type->info.element_len - 1);
                        break;
                    }
                    case STI_Array: {
                        fprintf(output,
                                "    print_%s(&obj->%s, lib, handle, indent+1);\n",
                                String_data(&member_type_name), member_name_cstr);
                        break;
                    }
                    case STI_Bitfield: {
                        fprintf(output, "    fprintf(handle, \"%%i\", obj->%s);\n",
                                member_name_cstr);
                        break;
                    }
                    case STI_StringHash: {
                        fprintf(output, "    print_%s(&obj->%s, lib, handle, indent+1);\n",
                                String_data(&member_type_name), member_name_cstr);
                        break;
                    }
                    case STI_Pointer: {
                        // assert(false && "Pointers are not supported yet");
                        fprintf(output,
                                "    assert(false && \"Pointers are not supported yet\");\n // Unknown how pointers work\n");
                        // Skip
                        break;
                    }
                    case STI_Enumeration: {
                        fprintf(output, "    fprintf(handle, \"%%s\", \"%s\");\n", String_data(&type_name));
                        fprintf(output, "    fprintf(handle, \" (%%i)\", (int)obj->%s);\n", member_name_cstr);
                        break;
                    }
                    case STI_Primitive: {
                    default: {
                        fprintf(output,
                                "    print_%s(&obj->%s, lib, handle, indent + 1);\n",
                                String_data(&member_type_name), member_name_cstr);
                        break;
                    }
                    }
                }
                fprintf(output, "    fprintf(handle, \"\\n\");\n");
                String_free(&member_type_name);
            }
            break;
        }
        case STI_Array: {
            STI_Type *inner_type = DM_get(&lib->types, type->info.element_type_hash);
            String inner_type_name = {};
            STI_get_type_name(lib, inner_type, &inner_type_name);
            fprintf(output,
                    "    if (obj->count>50) {\n"
                    "        fprintf(handle, \"%%*s\", indent*4, \"\");\n"
                    "        print_%s(&obj->items[0], lib, handle, indent+1);\n"
                    "        fprintf(handle, \"\\n%%*s...\\n%%*s\", indent*4, \"\", indent*4, \"\");\n"
                    "        print_%s(&obj->items[obj->count-1], lib, handle, indent+1);\n"
                    "        fprintf(handle, \"\\n\");\n"
                    "    } else {\n"
                    "        fprintf(handle, \"%%*s\", indent*4, \"\");\n"
                    "        for (uint32 i = 0; i < obj->count; ++i) {\n"
                    "            print_%s(&obj->items[i], lib, handle, indent+1);\n"
                    "            if(i < obj->count-1){\n"
                    "                fprintf(handle,\", \\n%%*s\", indent*4, \"\");\n"
                    "            }\n"
                    "        }\n"
                    "        fprintf(handle, \"\\n\");\n"
                    "    }\n",
                    String_data(&inner_type_name),
                    String_data(&inner_type_name),
                    String_data(&inner_type_name)
            );
            String_free(&inner_type_name);
            break;
        }
        case STI_Enumeration: {
            fprintf(output, "    fprintf(handle, \"%%*s%%s\", indent * 4, \"\", \"%s\");\n", String_data(&type_name));
            fprintf(output, "    fprintf(handle, \" (%%i)\", (int)*obj);\n");
            break;
        }
        case STI_DeferredType:
        case STI_Primitive:
        case STI_Pointer:
            // Nothing to print
            break;
        case STI_InlineArray:
        case STI_Bitfield:
        case STI_StringHash:
            break;
        default: {
            printf("Unknown type %i\n", type->type);
            assert(false && "Unknown type");
        }
    }
    fprintf(output, "    fprintf(handle, \"%%*s}\", (indent - 1) * 4, \"\");\n");
    fprintf(output, "}\n");
    String_free(&type_name);
}

void STI_generate_register_function(STI_TypeLibrary *lib, String *namespace, FILE *output) {
    fprintf(output, "void STI_%s_register_functions(STI_TypeLibrary* lib){\n",
            String_data(namespace));
    String type_name = {0};
    for (int i = 0; i < DM_count(&lib->types); ++i) {
        STI_Type *type = DM_get_value(&lib->types, i);

        if (DM_get(&lib->object_functions, type->info.hash) != NULL) {
            // Already known type
            continue;
        }

        if (type->type == STI_Primitive ||
            type->type == STI_DeferredType ||
            type->type == STI_Pointer ||
            type->type == STI_Bitfield ||
            type->type == STI_InlineArray ||
            type->type == STI_StringHash) {
            continue;
        }
        STI_get_type_name(lib, type, &type_name);
        fprintf(output, "    *((STI_ObjectMethods*)(DM_insert(&lib->object_functions, 0x%08X)))",
                type->info.hash);
        char *string_data = String_data(&type_name);
        fprintf(output,
                " = (STI_ObjectMethods){(readSTIObject)read_%s, (freeSTIObject)free_%s, (printSTIObject)print_%s, sizeof(%s)};\n",
                string_data, string_data, string_data, string_data);
    }
    fprintf(output, "}\n\n");

    fprintf(output, "for(uint32 ___i=0;___i<%i;___i++) {\n", lib->hash_strings.keys.count);
    fprintf(output, "    const char* str = STI_%s_hash_strings_string[___i];\n", String_data(namespace));
    fprintf(output, "    uint64 hash =     STI_%s_hash_strings_hash[___i];\n", String_data(namespace));
    fprintf(output, "    String* slot = DM_insert(&lib->hash_strings, hash);\n");
    fprintf(output, "    if(slot) String_from_cstr(slot, str);\n");
    fprintf(output, "}\n");

    String_free(&type_name);
}

void STI_dump_type(STI_TypeLibrary *lib, STI_Type *type, FILE *output) {
    if (DA_contains(&lib->exported_hashes, &type->info.hash, compare_hashes)) {
        return;
    }
    DA_append(&lib->exported_hashes, &type->info.hash);
    switch (type->type) {
        case STI_Structure: {
            DynamicArray_STI_StructMember *members = &type->type_data.struct_data.members;
            for (int i = 0; i < members->count; ++i) {
                STI_dump_type(lib, DM_get(&lib->types, members->items[i].info.type_hash), output);
            }
            fprintf(output, "#define STI_TYPE_HASH_%s 0x%08X\n", String_data(&type->name), type->info.hash);
            fprintf(output, "typedef struct %s{\n", String_data(&type->name));
            String type_name = {0};
            for (int i = 0; i < members->count; ++i) {
                STI_StructMember *member = &members->items[i];
                STI_Type *member_type = DM_get(&lib->types, member->info.type_hash);
                STI_get_type_name(lib, member_type, &type_name);
                switch (member_type->type) {
                    case STI_InlineArray: {
                        // STI_Type *inner_type = DM_get(&lib->types, member_type->info.element_type_hash);
                        // assert(inner_type->type==STI_Primitive&&"Only primitive inline arrays are supported.");
                        fprintf(output, "    %s %s[%i];", String_data(&type_name),
                                String_data(&member->name), member_type->info.element_len);
                        break;
                    }
                    case STI_Bitfield: {
                        fprintf(output, "    %s %s:%i;", String_data(&type_name),
                                String_data(&member->name), member_type->info.element_len);
                        break;
                    }
                    default: {
                        fprintf(output, "    %s %s;", String_data(&type_name), String_data(&member->name));
                        break;
                    }
                }
                fprintf(output, " // offset: %i, size: %i\n", member->info.offset, member_type->info.size);
            }
            fprintf(output, "} %s; // size: %i\n", String_data(&type->name), type->info.size);
            fprintf(output, "\n");
            String_free(&type_name);
            break;
        }
        case STI_Enumeration: {
            fprintf(output, "#define STI_TYPE_HASH_%s 0x%08X\n", String_data(&type->name), type->info.hash);
            fprintf(output, "typedef enum{ // size: %i\n", type->info.size);
            DynamicArray_STI_EnumMember members = type->type_data.enum_data.members;
            for (int i = 0; i < members.count; ++i) {
                fprintf(output, "    %s = %u,\n", String_data(&members.items[i].name), members.items[i].info.value);
            }
            fprintf(output, "} %s;\n", String_data(&type->name));
            fprintf(output, "\n");
            break;
        }
        case STI_Array: {
            String tmp = {0};
            STI_Type *inner_array_type = DM_get(&lib->types, type->info.element_type_hash);
            STI_dump_type(lib, inner_array_type, output);
            STI_get_type_name(lib, inner_array_type, &tmp);
            fprintf(output, "#define STI_TYPE_HASH_ARRAY_%s 0x%08X\n", String_data(&tmp), type->info.hash);
            fprintf(output, "DYNAMIC_ARRAY_STRUCT(%s, %s);\n", String_data(&tmp), String_data(&tmp));
            fprintf(output, "\n");
            String_free(&tmp);
            break;
        }
        case STI_DeferredType:
        case STI_Primitive: {
            break;
        }
        case STI_InlineArray: {
            STI_Type *inner_array_type = DM_get(&lib->types, type->info.element_type_hash);
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
}


void STI_TypeLibrary_free(STI_TypeLibrary *lib) {
    for (int i = 0; i < DM_count(&lib->types); ++i) {
        STI_Type *type = DM_get_value(&lib->types, i);
        STI_Type_free(type);
    }

    DM_free(&lib->types);
    DM_free(&lib->object_functions);
    DM_free(&lib->name_hash_to_type);
    DA_free(&lib->exported_hashes);
}

void STI_Type_free(STI_Type *type) {
    String_free(&type->name);
    switch (type->type) {
        case STI_Structure: {
            DA_free_with_inner(&type->type_data.struct_data.members, {String_free(&((STI_StructMember*)it)->name);});
            break;
        }
        case STI_Enumeration: {
            for (int i = 0; i < type->type_data.enum_data.members.count; ++i) {
                String_free(&type->type_data.enum_data.members.items[i].name);
            }
            DA_free(&type->type_data.enum_data.members);
            break;
        }
        case STI_Primitive:
        case STI_DeferredType:
        case STI_Array:
        case STI_InlineArray:
        case STI_Bitfield:
        case STI_StringHash:
        case STI_Pointer: {
            break;
        }
        default: {
            printf("Unknown type %i\n", type->type);
            assert(false && "Unknown type");
        };
    }
}

void STI_Type_init(STI_Type *type, STI_MetaType meta_type) {
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
        case STI_DeferredType:
        case STI_InlineArray: {
            break;
        }
        default: {
            printf("Unknown type %i\n", meta_type);
            assert(false && "Should not reach");
        };
    }
}

void STI_generate_struct_forward_declaration(STI_TypeLibrary *lib, STI_Type *type, FILE *output) {
    if (type->type != STI_Structure) {
        return;
    }

    String type_name = {0};
    STI_get_type_name(lib, type, &type_name);
    fprintf(output, "typedef struct %s %s;// size: %i\n", String_data(&type_name), String_data(&type_name),
            type->info.size);
    String_free(&type_name);
}

void STI_generate_array_forward_declaration(STI_TypeLibrary *lib, STI_Type *type, FILE *output) {
    if (type->type != STI_Array) {
        return;
    }

    String inner_type_name = {0};
    STI_Type *inner_type = DM_get(&lib->types, type->info.element_type_hash);
    STI_get_type_name(lib, inner_type, &inner_type_name);
    const char *inner_type_name_cstr = String_data(&inner_type_name);
    fprintf(output,
            "#define STI_TYPE_HASH_ARRAY_%s 0x%08X \nDYNAMIC_ARRAY_STRUCT(%s, %s);\n\n",
            inner_type_name_cstr, type->info.hash, inner_type_name_cstr, inner_type_name_cstr);
    String_free(&inner_type_name);
    DA_append(&lib->exported_hashes, &type->info.hash);
}

void STI_TypeLibrary_generate_types(STI_TypeLibrary *lib, String *namespace, FILE *header_output,
                                    String *relative_header_path, FILE *impl_output) {
    fprintf(header_output, "// This file is autogenerated\n");
    fprintf(header_output, "#ifndef %s_GUARD\n", String_data(namespace));
    fprintf(header_output, "#define %s_GUARD\n\n", String_data(namespace));
    fprintf(header_output, "#include \"apex/adf/sti.h\"\n");
    fprintf(header_output, "#include \"apex/adf/sti_shared.h\"\n");
    fprintf(header_output, "#include \"utils/dynamic_array.h\"\n");
    fprintf(header_output, "#include \"utils/lookup3.h\"\n");

    fprintf(header_output, "void STI_%s_register_functions(STI_TypeLibrary* lib);\n\n",
            String_data(namespace));


    fprintf(impl_output, "// This file is autogenerated\n");
    fprintf(impl_output, "#include \"%s\"\n\n", String_data(relative_header_path));
    fprintf(impl_output, "#include \"utils/dynamic_insert_only_map.h\"\n");
    fprintf(impl_output, "#include <assert.h>\n");

    for (int32 i = STI_TypeLibrary_types_count(lib) - 1; i >= 0; --i) {
        STI_Type *type = DM_get_value(&lib->types, i);
        STI_generate_struct_forward_declaration(lib, type, header_output);
    }
    fprintf(header_output, "\n");
    for (int32 i = STI_TypeLibrary_types_count(lib) - 1; i >= 0; --i) {
        STI_Type *type = DM_get_value(&lib->types, i);
        STI_generate_array_forward_declaration(lib, type, header_output);
    }
    fprintf(header_output, "\n");
    for (int32 i = STI_TypeLibrary_types_count(lib) - 1; i >= 0; --i) {
        STI_Type *type = DM_get_value(&lib->types, i);
        STI_dump_type(lib, type, header_output);
    }

    for (int32 i = STI_TypeLibrary_types_count(lib) - 1; i >= 0; --i) {
        STI_Type *type = DM_get_value(&lib->types, i);
        STI_generate_reader_function(lib, type, impl_output, true);
        STI_generate_free_function(lib, type, impl_output, true);
        STI_generate_print_function(lib, type, impl_output, true);
    }
    fprintf(impl_output, "\n\n");
    for (int32 i = STI_TypeLibrary_types_count(lib) - 1; i >= 0; --i) {
        STI_Type *type = DM_get_value(&lib->types, i);
        STI_generate_reader_function(lib, type, impl_output, false);
        STI_generate_free_function(lib, type, impl_output, false);
        STI_generate_print_function(lib, type, impl_output, false);
    }
    STI_generate_register_function(lib, namespace, impl_output);

    fprintf(header_output, "extern const char*  STI_%s_hash_strings_string[%i];\n\n", String_data(namespace),
            lib->hash_strings.keys.count);
    fprintf(header_output, "extern const uint64 STI_%s_hash_strings_hash[%i];\n\n", String_data(namespace),
            lib->hash_strings.keys.count);
    fprintf(impl_output, "const char* STI_%s_hash_strings_string[%i] = {", String_data(namespace),
            lib->hash_strings.keys.count);
    for (int i = 0; i < lib->hash_strings.keys.count; ++i) {
        if (i % 10 == 0)fprintf(impl_output, "\n");
        fprintf(impl_output, "\"%s\", ", String_data(&lib->hash_strings.values.items[i]));
    }
    fprintf(impl_output, "\n};\n");
    fprintf(impl_output, "const uint64 STI_%s_hash_strings_hash[%i] = {", String_data(namespace),
            lib->hash_strings.keys.count);
    for (int i = 0; i < lib->hash_strings.keys.count; ++i) {
        if (i % 10 == 0)fprintf(impl_output, "\n");
        fprintf(impl_output, "%llu, ", lib->hash_strings.keys.items[i]);
    }
    fprintf(impl_output, "\n};\n");
    fprintf(header_output, "#endif //%s_GUARD\n", String_data(namespace));
}
