// Created by RED on 07.10.2025.
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils/common.h"
#include "utils/lookup3.h"
#include "apex/adf/sti.h"
#include "utils/string.h"


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

void STI_get_type_name(const STI_TypeLibrary *lib, const STI_Type *type, String *type_name) {
    switch (type->type) {
        case STI_Structure:
        case STI_Enumeration: {
            String_copy_from(type_name, &type->name);
            break;
        }
        case STI_DeferredType:
        case STI_Pointer:
        case STI_Primitive: {
            String_from_cstr(type_name, "STI_");
            String_append_str(type_name, &type->name);
            String_from_cstr(type_name, "STI_");
            String_append_str(type_name, &type->name);
            break;
        }
        case STI_Array: {
            const STI_Type *inner_array_type = STI_TypeLibrary_get_type(lib, type->info.element_type_hash);
            String tmp = {0};
            String_from_cstr(&tmp, "DynamicArray_");
            STI_get_type_name(lib, inner_array_type, type_name);
            String_append_str(&tmp, type_name);
            String_copy_from(type_name, String_move(&tmp));
            String_free(&tmp);
            break;
        }
        case STI_InlineArray: {
            const STI_Type *inner_array_type = STI_TypeLibrary_get_type(lib, type->info.element_type_hash);
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
                case 0x18db7671: {
                    String_from_cstr(type_name, "StringHash_48c5294d_8");
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

void STI_generate_reader_function(STI_TypeLibrary *lib, const STI_Type *type, FILE *output, bool prototype_only) {
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
            const DynamicArray_STI_StructMember *members = &type->type_data.struct_data.members;
            uint32 running_offset = 0;
            String member_type_name = {0};
            for (uint32 i = 0; i < members->count; ++i) {
                const STI_StructMember *member = &members->items[i];
                const STI_Type *member_type = STI_TypeLibrary_get_type(lib, member->info.type_hash);
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
                        const STI_Type *total_type = STI_TypeLibrary_get_type(lib, members->items[i].info.type_hash);
                        for (uint32 j = i; j < members->count; j++) {
                            const STI_StructMember *bit_member = &members->items[j];
                            const STI_Type *bit_member_type = STI_TypeLibrary_get_type(lib, bit_member->info.type_hash);
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
                            const STI_StructMember *bit_member = &members->items[j];
                            const STI_Type *bit_member_type = STI_TypeLibrary_get_type(lib, bit_member->info.type_hash);
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
            const STI_Type *inner_array_type = STI_TypeLibrary_get_type(lib, type->info.element_type_hash);
            String inner_array_type_name = {0};
            STI_get_type_name(lib, inner_array_type, &inner_array_type_name);
            const char *inner_type_name_cstr = String_data(&inner_array_type_name);
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

void STI_generate_free_function(STI_TypeLibrary *lib, const STI_Type *type, FILE *output, bool prototype_only) {
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

    const char *type_name_cstr = String_data(&type_name);
    if (prototype_only) {
        fprintf(output, "static void free_%s(%s* obj, STI_TypeLibrary* lib);\n", type_name_cstr, type_name_cstr);
        String_free(&type_name);
        return;
    }
    fprintf(output, "static void free_%s(%s* obj, STI_TypeLibrary* lib) {\n", type_name_cstr, type_name_cstr);
    switch (type->type) {
        case STI_Structure: {
            const DynamicArray_STI_StructMember *members = &type->type_data.struct_data.members;
            String member_type_name = {0};
            for (uint32 i = 0; i < members->count; ++i) {
                const STI_StructMember *member = &members->items[i];
                const STI_Type *member_type = STI_TypeLibrary_get_type(lib, member->info.type_hash);
                STI_get_type_name(lib, member_type, &member_type_name);
                switch (member_type->type) {
                    case STI_InlineArray: {
                        // Inline arrays do need inner items freed
                        const STI_Type *inner_type = STI_TypeLibrary_get_type(lib, member_type->info.element_type_hash);
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
            const STI_Type *inner_type = STI_TypeLibrary_get_type(lib, type->info.element_type_hash);
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

void STI_generate_print_function(STI_TypeLibrary *lib, const STI_Type *type, FILE *output, bool prototype_only) {
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
    const char *type_name_cstr = String_data(&type_name);
    if (prototype_only) {
        fprintf(output, "static void print_%s(const %s* obj, STI_TypeLibrary* lib, FILE* handle, uint32 indent);\n",
                type_name_cstr, type_name_cstr);
        String_free(&type_name);
        return;
    }

    fprintf(output, "static void print_%s(const %s* obj, STI_TypeLibrary* lib, FILE* handle, uint32 indent) {\n",
            type_name_cstr, type_name_cstr);

    switch (type->type) {
        case STI_Structure: {
            fprintf(output, "    fprintf(handle, \"%s {\\n\");\n    indent++;\n", type_name_cstr);
            const DynamicArray_STI_StructMember *members = &type->type_data.struct_data.members;
            String member_type_name = {0};
            for (uint32 i = 0; i < members->count; ++i) {
                const STI_StructMember *member = &members->items[i];
                const STI_Type *member_type = STI_TypeLibrary_get_type(lib, member->info.type_hash);
                STI_get_type_name(lib, member_type, &member_type_name);
                const char *member_name_cstr = String_data(&member->name);
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
                    case STI_Bitfield: {
                        fprintf(output, "    fprintf(handle, \"%%i\", obj->%s);\n",
                                member_name_cstr);
                        break;
                    }
                    case STI_Pointer: {
                        // assert(false && "Pointers are not supported yet");
                        fprintf(output,
                                "    assert(false && \"Pointers are not supported yet\");\n // Unknown how pointers work\n");
                        // Skip
                        break;
                    }
                    case STI_Enumeration:
                    case STI_StringHash:
                    case STI_Primitive:
                    case STI_Array:
                    default: {
                        fprintf(output,
                                "    print_%s(&obj->%s, lib, handle, indent + 1);\n",
                                String_data(&member_type_name), member_name_cstr);
                        break;
                    }
                }
                fprintf(output, "    fprintf(handle, \"\\n\");\n");
                String_free(&member_type_name);
            }
            fprintf(output, "    fprintf(handle, \"%%*s}\", (indent - 1)*4, \"\");\n");
            break;
        }
        case STI_Array: {
            fprintf(output, "    fprintf(handle, \"%s {\\n\");\n    indent++;\n", type_name_cstr);
            const STI_Type *inner_type = STI_TypeLibrary_get_type(lib, type->info.element_type_hash);
            String inner_type_name = {};
            STI_get_type_name(lib, inner_type, &inner_type_name);
            fprintf(output,
                    "    if (obj->count>50) {\n"
                    "        fprintf(handle, \"%%*s\", indent * 4, \"\");\n"
                    "        print_%s(&obj->items[0], lib, handle, indent + 1);\n"
                    "        fprintf(handle, \"\\n%%*s...\\n%%*s\", indent * 4, \"\", indent * 4, \"\");\n"
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
            fprintf(output, "    fprintf(handle, \"%%*s}\", (indent - 1) * 4, \"\");\n");
            String_free(&inner_type_name);
            break;
        }
        case STI_Enumeration: {
            fprintf(output, "    fprintf(handle, \"%s( \");\n    indent++;\n", type_name_cstr);
            fprintf(output, "    switch(*obj) {\n");
            for (int i = 0; i < type->type_data.enum_data.members.count; ++i) {
                const STI_EnumMember *enum_member = &type->type_data.enum_data.members.items[i];
                fprintf(output, "        case(%s): {\n", String_data(&enum_member->name));
                fprintf(output, "            fprintf(handle, \"%s )\");\nbreak;\n", String_data(&enum_member->name));
                fprintf(output, "            break;\n");
                fprintf(output, "        }\n");
            }
            fprintf(output, "    }\n");
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

    fprintf(output, "}\n");
    String_free(&type_name);
}

void STI_generate_register_function(STI_TypeLibrary *lib, const String *namespace, FILE *output) {
    fprintf(output, "void STI_%s_register_functions(STI_TypeLibrary* lib){\n", String_data(namespace));
    String type_name = {0};
    for (int i = 0; i < DM_count(&lib->types); ++i) {
        const STI_Type *type = DM_get_value(&lib->types, i);

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
        const char *string_data = String_data(&type_name);
        fprintf(output,
                " = (STI_ObjectMethods){(readSTIObject)read_%s, (freeSTIObject)free_%s, (printSTIObject)print_%s, sizeof(%s)};\n",
                string_data, string_data, string_data, string_data);
    }

    fprintf(output, "for(uint32 ___i=0;___i<%i;___i++) {\n", lib->hash_strings.keys.count);
    fprintf(output, "    const char* str = STI_%s_hash_strings_string[___i];\n", String_data(namespace));
    fprintf(output, "    uint64 hash =     STI_%s_hash_strings_hash[___i];\n", String_data(namespace));
    fprintf(output, "    String* slot = DM_insert(&lib->hash_strings, hash);\n");
    fprintf(output, "    if(slot) String_from_cstr(slot, str);\n");
    fprintf(output, "}\n");

    fprintf(output, "}\n\n");
    String_free(&type_name);
}

void STI_dump_type(STI_TypeLibrary *lib, const STI_Type *type, FILE *output) {
    if (DA_contains(&lib->exported_hashes, &type->info.hash, compare_hashes)) {
        return;
    }
    DA_append(&lib->exported_hashes, &type->info.hash);
    switch (type->type) {
        case STI_Structure: {
            const DynamicArray_STI_StructMember *members = &type->type_data.struct_data.members;
            for (int i = 0; i < members->count; ++i) {
                STI_dump_type(lib, DM_get(&lib->types, members->items[i].info.type_hash), output);
            }
            fprintf(output, "#define STI_TYPE_HASH_%s 0x%08X\n", String_data(&type->name), type->info.hash);
            fprintf(output, "typedef struct %s{\n", String_data(&type->name));
            String type_name = {0};
            for (int i = 0; i < members->count; ++i) {
                const STI_StructMember *member = &members->items[i];
                const STI_Type *member_type = STI_TypeLibrary_get_type(lib, member->info.type_hash);
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
            const DynamicArray_STI_EnumMember members = type->type_data.enum_data.members;
            for (int i = 0; i < members.count; ++i) {
                fprintf(output, "    %s = %u,\n", String_data(&members.items[i].name), members.items[i].info.value);
            }
            fprintf(output, "} %s;\n", String_data(&type->name));
            fprintf(output, "\n");
            break;
        }
        case STI_Array: {
            String tmp = {0};
            const STI_Type *inner_array_type = STI_TypeLibrary_get_type(lib, type->info.element_type_hash);
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
            const STI_Type *inner_array_type = STI_TypeLibrary_get_type(lib, type->info.element_type_hash);
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


void STI_generate_struct_forward_declaration(const STI_TypeLibrary *lib, const STI_Type *type, FILE *output) {
    if (type->type != STI_Structure) {
        return;
    }

    String type_name = {0};
    STI_get_type_name(lib, type, &type_name);
    fprintf(output, "typedef struct %s %s;// size: %i\n", String_data(&type_name), String_data(&type_name),
            type->info.size);
    String_free(&type_name);
}

void STI_generate_array_forward_declaration(STI_TypeLibrary *lib, const STI_Type *type, FILE *output) {
    if (type->type != STI_Array) {
        return;
    }

    String inner_type_name = {0};
    const STI_Type *inner_type = STI_TypeLibrary_get_type(lib, type->info.element_type_hash);
    STI_get_type_name(lib, inner_type, &inner_type_name);
    const char *inner_type_name_cstr = String_data(&inner_type_name);
    fprintf(output,
            "#define STI_TYPE_HASH_ARRAY_%s 0x%08X \nDYNAMIC_ARRAY_STRUCT(%s, %s);\n\n",
            inner_type_name_cstr, type->info.hash, inner_type_name_cstr, inner_type_name_cstr);
    String_free(&inner_type_name);
    DA_append(&lib->exported_hashes, &type->info.hash);
}

void STI_TypeLibrary_generate_types(STI_TypeLibrary *lib, const String *namespace, FILE *header_output,
                                    const String *relative_header_path, FILE *impl_output) {
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
        const STI_Type *type = DM_get_value(&lib->types, i);
        STI_generate_struct_forward_declaration(lib, type, header_output);
    }
    fprintf(header_output, "\n");
    for (int32 i = STI_TypeLibrary_types_count(lib) - 1; i >= 0; --i) {
        const STI_Type *type = DM_get_value(&lib->types, i);
        STI_generate_array_forward_declaration(lib, type, header_output);
    }
    fprintf(header_output, "\n");
    for (int32 i = STI_TypeLibrary_types_count(lib) - 1; i >= 0; --i) {
        const STI_Type *type = DM_get_value(&lib->types, i);
        STI_dump_type(lib, type, header_output);
    }

    for (int32 i = STI_TypeLibrary_types_count(lib) - 1; i >= 0; --i) {
        const STI_Type *type = DM_get_value(&lib->types, i);
        STI_generate_reader_function(lib, type, impl_output, true);
        STI_generate_free_function(lib, type, impl_output, true);
        STI_generate_print_function(lib, type, impl_output, true);
    }
    fprintf(impl_output, "\n\n");
    for (int32 i = STI_TypeLibrary_types_count(lib) - 1; i >= 0; --i) {
        const STI_Type *type = DM_get_value(&lib->types, i);
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
        fprintf(impl_output, "%lu, ", lib->hash_strings.keys.items[i]);
    }
    fprintf(impl_output, "\n};\n");
    fprintf(header_output, "#endif //%s_GUARD\n", String_data(namespace));
}
