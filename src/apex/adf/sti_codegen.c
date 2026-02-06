// // Created by RED on 07.10.2025.
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils/common.h"
#include "utils/lookup3.h"
#include "apex/adf/sti.h"
#include "platform/logger.h"
#include "utils/string.h"

void STI_dump_type(STI_TypeLibrary *lib, const STI_Type *type, FILE *output);

void STI_generate_init_function(const STI_TypeLibrary *lib, const STI_Type *type, FILE *output);

void STI_generate_read_function(const STI_TypeLibrary *lib, const STI_Type *type, FILE *output);

void STI_generate_free_function(const STI_TypeLibrary *lib, const STI_Type *type, FILE *output);

void STI_generate_print_function(const STI_TypeLibrary *lib, const STI_Type *type, FILE *output);

void STI_generate_register_function(STI_TypeLibrary *lib, const String *namespace, FILE *output);


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


bool is_complex_type(const STI_TypeLibrary *lib, const STI_Type *type) {
    bool res = false;
    if (type->type == STI_Structure) {
        for (int i = 0; i < type->data.struct_data.members.count; ++i) {
            const STI_Type *member_type = DM_get(&lib->types, type->data.struct_data.members.items[i].type_hash);
            if (member_type->type == STI_Array) {
                return true;
            }
            res |= is_complex_type(lib, member_type);
        }
    }
    else if (type->type == STI_InlineArray) {
        const STI_Type *inner_type = DM_get(&lib->types, type->data.array_data.type_hash);
        res |= is_complex_type(lib, inner_type);
    }
    else if (type->type == STI_Pointer) {
        const STI_Type *inner_type = DM_get(&lib->types, type->data.deferred_data.type_hash);
        res |= is_complex_type(lib, inner_type);
    }
    else if (type->type == STI_Alias) {
        const STI_Type *inner_type = DM_get(&lib->types, type->data.deferred_data.type_hash);
        res |= is_complex_type(lib, inner_type);
    }
    else if (type->type == STI_Enumeration ||
             type->type == STI_Primitive ||
             type->type == STI_Bitfield ||
             type->type == STI_StringHash) {
        res = false;
    }
    else {
        res = true;
    }
    return res;
}


void STI_dump_type(STI_TypeLibrary *lib, const STI_Type *type, FILE *output) {
    if (DA_contains(&lib->exported_hashes, &type->hash, compare_hashes)) {
        return;
    }
    DA_append(&lib->exported_hashes, &type->hash);
    switch (type->type) {
        case STI_Structure: {
            const DynamicArray_STI_StructMember *members = &type->data.struct_data.members;
            for (int i = 0; i < members->count; ++i) {
                const STI_StructMember *member = &members->items[i];
                const STI_Type *member_type = DM_get(&lib->types, member->type_hash);
                if (member_type == NULL) {
                    GLog_Error("Failed to find type with hash 0x%08X", member->type_hash);
                    continue;
                }
                STI_dump_type(lib, member_type, output);
            }
            fprintf(output, "#define STI_TYPE_HASH_%s 0x%08X\n", String_cstr(&type->name), type->hash);
            fprintf(output, "typedef struct %s{\n", String_cstr(&type->name));
            fprintf(output, "    const STITypeInfo* type_info_;\n");
            for (int i = 0; i < members->count; ++i) {
                const STI_StructMember *member = &members->items[i];
                const STI_Type *member_type = STI_TypeLibrary_get_type(lib, member->type_hash);
                const String *member_name = &member->name;
                const String *type_name = &member_type->name;


                switch (member_type->type) {
                    case STI_InlineArray: {
                        const STI_Type *inner_type = STI_TypeLibrary_get_type(
                            lib, member_type->data.array_data.type_hash);
                        fprintf(output, "    %s %s[%i];", String_cstr(&inner_type->name),
                                String_cstr(member_name), member_type->data.array_data.count);
                        break;
                    }
                    case STI_Array: {
                        fprintf(output, "    %s %s;", String_cstr(&member_type->name),
                                String_cstr(member_name));
                        break;
                    }
                    default: {
                        fprintf(output, "    %s %s;", String_cstr(type_name), String_cstr(member_name));
                        break;
                    }
                }
                fprintf(output, " // offset: %i, size: %i\n", member->offset, member_type->size);
            }
            fprintf(output, "} %s; // size: %i\n", String_cstr(&type->name), type->size);
            fprintf(output, "\n");
            break;
        }
        case STI_Enumeration: {
            fprintf(output, "#define STI_TYPE_HASH_%s 0x%08X\n", String_cstr(&type->name), type->hash);
            fprintf(output, "typedef enum{ // size: %i\n", type->size);
            const DynamicArray_STI_EnumMember members = type->data.enum_data.members;
            for (int i = 0; i < members.count; ++i) {
                fprintf(output, "    %s = %u,\n", String_cstr(&members.items[i].name), members.items[i].value);
            }
            const uint32 size_force_value = (1 << (type->size * 8 - 1))-1;
            fprintf(output, "    %s_ForceSize = 0x%08X\n", String_cstr(&type->name), size_force_value);
            fprintf(output, "} %s;\n", String_cstr(&type->name));
            fprintf(output, "\n");
            break;
        }
        case STI_Array: {
            const STI_Type *inner_array_type = STI_TypeLibrary_get_type(lib, type->data.array_data.type_hash);
            fprintf(output, "#define STI_TYPE_HASH_%s 0x%08X\n", String_cstr(&type->name), type->hash);
            fprintf(output, "typedef struct %s {\n", String_cstr(&type->name));
            fprintf(output, "    const STITypeInfo* type_info_;\n");
            fprintf(output, "    uint32 count;\n");
            fprintf(output, "    %s* items;\n", String_cstr(&inner_array_type->name));
            fprintf(output, "} %s; // size: %i\n", String_cstr(&type->name), type->size);
            fprintf(output, "\n");
            // STI_dump_type(lib, inner_array_type, output);
            break;
        }
        case STI_DeferredType:
        case STI_Primitive: {
            break;
        }
        case STI_InlineArray: {
            const STI_Type *inner_array_type = STI_TypeLibrary_get_type(lib, type->data.array_data.type_hash);
            STI_dump_type(lib, inner_array_type, output);
            break;
        }
        case STI_StringHash:
        case STI_Bitfield:
        case STI_Alias:
        case STI_Pointer: {
            return;
        }
        default: {
            printf("Unknown type %i\n", type->type);
            assert(false && "Unknown type");
        }
        fprintf(output, "extern STITypeInfo %s_TI;\n", String_cstr(&type->name));

    }
}

void STI_generate_init_function(const STI_TypeLibrary *lib, const STI_Type *type, FILE *output) {
    const bool need_init = is_complex_type(lib, type) || type->type == STI_Array || type->type == STI_Structure;
    if (!need_init || type->type == STI_InlineArray || type->type == STI_DeferredType || type->type == STI_Alias) {
        return;
    }

    printf("%s\n", String_cstr(&type->name));
    fprintf(output, "void %s_init(%s *obj) {\n", String_cstr(&type->name), String_cstr(&type->name));
    if (type->type == STI_Structure) {
        fprintf(output, "    obj->type_info_ = &%s_TI;\n", String_cstr(&type->name));
        const DynamicArray_STI_StructMember *members = &type->data.struct_data.members;
        for (int i = 0; i < members->count; ++i) {
            const STI_StructMember *member = &members->items[i];
            const STI_Type *member_type = STI_TypeLibrary_get_type(lib, member->type_hash);
            const String *member_name = &member->name;

            const bool member_is_complex = is_complex_type(lib, member_type);
            const bool member_need_init = member_is_complex || member_type->type == STI_Array || member_type->type ==
                                          STI_Structure;
            switch (member_type->type) {
                case STI_InlineArray: {
                    if (member_need_init) {
                        const STI_Type *inner_type = STI_TypeLibrary_get_type(
                            lib, member_type->data.array_data.type_hash);
                        fprintf(output, "    for(int i = 0; i < %i; ++i) {\n", member_type->data.array_data.count);
                        fprintf(output, "        %s_init(&obj->%s[i]);\n", String_cstr(&inner_type->name),
                                String_cstr(member_name));
                        fprintf(output, "    }\n");
                    }
                    break;
                }
                case STI_Array: {
                    fprintf(output, "    %s_init(&obj->%s);\n", String_cstr(&member_type->name),
                            String_cstr(member_name));
                    break;
                }
                default: {
                    if (member_need_init) {
                        fprintf(output, "    %s_init(&obj->%s);\n", String_cstr(&member_type->name),
                                String_cstr(member_name));
                    }
                    break;
                }
            }
        }
    }
    else if (type->type == STI_Array) {
        fprintf(output, "    obj->type_info_ = &%s_TI;\n", String_cstr(&type->name));
        fprintf(output, "    obj->count = 0;\n");
        fprintf(output, "    obj->items = NULL;\n");
    }
    fprintf(output, "}\n\n");
}

void STI_generate_read_function(const STI_TypeLibrary *lib, const STI_Type *type, FILE *output) {
    if (type->type == STI_InlineArray ||
        type->type == STI_Pointer ||
        type->type == STI_Primitive ||
        type->type == STI_Alias ||
        type->type == STI_Bitfield ||
        type->type == STI_StringHash ||
        type->type == STI_DeferredType
    ) {
        return;
    }
    fprintf(output,
            "bool %s_read(%s *obj, Buffer* buffer) {\n",
            String_cstr(&type->name), String_cstr(&type->name));
    if (type->type == STI_Structure) {
        const DynamicArray_STI_StructMember *members = &type->data.struct_data.members;
        uint32 running_offset = 0;
        for (uint32 i = 0; i < members->count; ++i) {
            const STI_StructMember *member = &members->items[i];
            const STI_Type *member_type = STI_TypeLibrary_get_type(lib, member->type_hash);
            const String *member_name = &member->name;

            if (running_offset != member->offset) {
                const uint32 pad_size = member->offset - running_offset;
                fprintf(output, "    buffer->skip(buffer, %i);\n", pad_size);
                running_offset += pad_size;
            }

            switch (member_type->type) {
                case STI_InlineArray: {
                    const STI_Type *inner_type = STI_TypeLibrary_get_type(
                        lib, member_type->data.array_data.type_hash);
                    fprintf(output, "    for(int i = 0; i < %i; ++i) {\n", member_type->data.array_data.count);
                    fprintf(output, "        %s_read(&obj->%s[i], buffer);\n", String_cstr(&inner_type->name),
                            String_cstr(member_name));
                    fprintf(output, "    }\n");
                    running_offset += member_type->size != 0 ? member_type->size : member->size;
                    break;
                }
                case STI_Bitfield: {
                    fprintf(output, "{\n");
                    const uint32 first_bit_member = i;
                    uint32 last_bit_member = i;
                    const STI_Type *total_type = STI_TypeLibrary_get_type(lib, members->items[i].type_hash);
                    for (uint32 j = i; j < members->count; j++) {
                        const STI_StructMember *bit_member = &members->items[j];
                        const STI_Type *bit_member_type = STI_TypeLibrary_get_type(lib, bit_member->type_hash);
                        if (bit_member_type->type == STI_Bitfield) {
                            last_bit_member = j;
                        }
                        else {
                            last_bit_member = j - 1;
                            break;
                        }
                    }
                    const String *bit_type_name = &total_type->name;
                    fprintf(output,
                            "        %s bitfield_value = 0;\n"
                            "        %s_read(&bitfield_value, buffer);\n",
                            String_cstr(bit_type_name),
                            String_cstr(bit_type_name));
                    uint32 bit_offset = 0;
                    for (uint32 j = first_bit_member; j <= last_bit_member; ++j) {
                        const STI_StructMember *bit_member = &members->items[j];
                        const STI_Type *bit_member_type = STI_TypeLibrary_get_type(lib, bit_member->type_hash);
                        fprintf(output,
                                "        obj->%s = (bitfield_value >> %i) & 0x%X;\n",
                                String_cstr(&bit_member->name), bit_offset,
                                (1u << bit_member_type->data.bits_data) - 1);
                        bit_offset += bit_member_type->data.bits_data;
                    }
                    running_offset += total_type->size;
                    i = last_bit_member;
                    fprintf(output, "}\n");
                    break;
                }
                case STI_Pointer: {
                    // assert(false && "Pointers are not supported yet");
                    fprintf(output,
                            "    assert(false && \"Pointers are not supported yet\"); // Unknown how pointers work\n");
                    running_offset += member_type->size != 0 ? member_type->size : member->size;
                    break;
                }
                default: {
                    fprintf(output, "    %s_read(&obj->%s, buffer);\n", String_cstr(&member_type->name),
                            String_cstr(member_name));
                    running_offset += member_type->size != 0 ? member_type->size : member->size;
                    break;
                }
            }
        }
        if (running_offset != type->size) {
            const uint32 pad_size = type->size - running_offset;
            fprintf(output, "    buffer->skip(buffer, %i);\n", pad_size);
        }
    }
    else if (type->type == STI_Array) {
        const STI_Type *inner_array_type = STI_TypeLibrary_get_type(lib, type->data.array_data.type_hash);
        const bool is_inner_complex = inner_array_type->type == STI_Structure | inner_array_type->type == STI_Array;
        fprintf(output,
                "    uint32 count = 0;\n"
                "    uint32 unk0 = 0;\n"
                "    uint32 offset = 0;\n"
                "    uint32 unk1 = 0;\n"
                "    if (!uint32_read(&offset, buffer)) return false;\n"
                "    if (!uint32_read(&unk0, buffer)) return false;\n"
                "    if (!uint32_read(&count, buffer)) return false;\n"
                "    if (!uint32_read(&unk1, buffer)) return false;\n"
                "    obj->count = count;\n"
                "    if(count>0){\n"
                "       int64 original_offset = 0;\n"
                "       obj->items = mp_calloc(sizeof(%s), count);\n"
                "       if(buffer->get_position(buffer, &original_offset)!=BUFFER_SUCCESS) return false;\n"
                "       if(buffer->set_position(buffer, offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;\n"
                "       for (uint32 i = 0; i < count; ++i) {\n",
                String_cstr(&inner_array_type->name)
        );
        if (is_inner_complex) {
            fprintf(output,
                    "           %s_init(&obj->items[i]);\n",
                    String_cstr(&inner_array_type->name)
            );
        }
        fprintf(output,
                "           %s_read(&obj->items[i], buffer);\n"
                "       }\n"
                "       if(buffer->set_position(buffer, original_offset, BUFFER_ORIGIN_START)!=BUFFER_SUCCESS) return false;\n"
                "    }\n",
                String_cstr(&inner_array_type->name)
        );
    }
    else if (type->type == STI_Enumeration) {
        if (type->size == 4)
            fprintf(output, "    if (!uint32_read((uint32*)obj, buffer)) return false;\n");
        else if (type->size == 2)
            fprintf(output, "    if (!uint16_read((uint16*)obj, buffer)) return false;\n");
        else if (type->size == 1)
            fprintf(output, "    if (!uint8_read((uint8*)obj, buffer)) return false;\n");
        else
            assert(false && "Unhandled enum size");
    }
    else {
        assert(false && "Unhandled");
    }
    fprintf(output, "    return true;\n");
    fprintf(output, "}\n\n");
}

void STI_generate_free_function(const STI_TypeLibrary *lib, const STI_Type *type, FILE *output) {
    const bool is_complex = is_complex_type(lib, type);
    if (!is_complex || type->type == STI_InlineArray || type->type == STI_DeferredType || type->type == STI_Alias) {
        return;
    }

    fprintf(output, "void %s_free(%s *obj) {\n", String_cstr(&type->name), String_cstr(&type->name));
    if (type->type == STI_Structure) {
        const DynamicArray_STI_StructMember *members = &type->data.struct_data.members;
        for (int i = 0; i < members->count; ++i) {
            const STI_StructMember *member = &members->items[i];
            const STI_Type *member_type = STI_TypeLibrary_get_type(lib, member->type_hash);
            const String *member_name = &member->name;

            const bool member_is_complex = is_complex_type(lib, member_type);
            if (member_is_complex) {
                switch (member_type->type) {
                    case STI_InlineArray: {
                        const STI_Type *inner_type = STI_TypeLibrary_get_type(
                            lib, member_type->data.array_data.type_hash);
                        fprintf(output, "    for(int i = 0; i < %i; ++i) {\n", member_type->data.array_data.count);
                        fprintf(output, "        %s_free(&obj->%s[i]);\n", String_cstr(&inner_type->name),
                                String_cstr(member_name));
                        fprintf(output, "    }\n");
                        break;
                    }
                    default: {
                        fprintf(output, "    %s_free(&obj->%s);\n", String_cstr(&member_type->name),
                                String_cstr(member_name));
                        break;
                    }
                }
            }
        }
    }
    else if (type->type == STI_Array) {
        const STI_Type *inner_type = STI_TypeLibrary_get_type(lib, type->data.array_data.type_hash);
        if (is_complex_type(lib, inner_type)) {
            fprintf(output, "    for (uint32 i = 0; i < obj->count; ++i) {\n");
            fprintf(output, "        %s_free(&obj->items[i]);\n", String_cstr(&inner_type->name));
            fprintf(output, "    }\n");
        }

        fprintf(output, "    mp_free(obj->items);\n");
    }
    fprintf(output, "}\n\n");
}

void STI_generate_print_function(const STI_TypeLibrary *lib, const STI_Type *type, FILE *output) {
    if (type->type == STI_Primitive ||
        type->type == STI_DeferredType ||
        type->type == STI_Pointer ||
        type->type == STI_Bitfield ||
        type->type == STI_StringHash ||
        type->type == STI_Alias ||
        type->type == STI_InlineArray
    ) {
        return;
    }

    fprintf(output, "void %s_print(%s *obj, JsonContext* ctx) {\n",
            String_cstr(&type->name), String_cstr(&type->name));

    if (type->type == STI_Structure) {
        fprintf(output, "    jsonBeginObject(ctx);\n");
        const DynamicArray_STI_StructMember *members = &type->data.struct_data.members;
        for (int i = 0; i < members->count; ++i) {
            const STI_StructMember *member = &members->items[i];
            const STI_Type *member_type = STI_TypeLibrary_get_type(lib, member->type_hash);
            const String *member_name = &member->name;

            fprintf(output, "    jsonName(ctx, \"%s\");\n", String_cstr(member_name));
            if (member_type->type == STI_InlineArray) {
                const STI_Type *inner_type = STI_TypeLibrary_get_type(
                    lib, member_type->data.array_data.type_hash);
                fprintf(output,
                        "    jsonBeginCompactObject(ctx);\n"
                        "    for (uint32 i = 0; i < %i; ++i) {\n"
                        "        %s_print(&obj->%s[i], ctx);\n"
                        "    }\n"
                        "    jsonEndCompactArray(ctx);\n",
                        member_type->data.array_data.count,
                        String_cstr(&inner_type->name),
                        String_cstr(member_name)
                );
            }
            else if (member_type->type == STI_Pointer) {
                fprintf(output,
                        "    assert(false && \"Pointers are not supported yet\"); // Unknown how pointers work\n");
            }
            else {
                fprintf(output, "    %s_print(&obj->%s, ctx);\n", String_cstr(&member_type->name),
                        String_cstr(member_name));
            }
        }
        fprintf(output, "    jsonEndObject(ctx);\n");
    }
    else if (type->type == STI_Array) {
        const STI_Type *inner_array_type = STI_TypeLibrary_get_type(lib, type->data.array_data.type_hash);
        fprintf(output,
                "    jsonBeginArray(ctx);\n"
                "    for (uint32 i = 0; i < obj->count; ++i) {\n"
                "        %s_print(&obj->items[i], ctx);\n"
                "    }\n"
                "    jsonEndArray(ctx);\n",
                String_cstr(&inner_array_type->name)
        );
    }
    fprintf(output, "}\n\n");
}

void STI_generate_register_function(STI_TypeLibrary *lib, const String *namespace, FILE *output) {
    fprintf(output,
            "static inline void register_type_info(STITypeInfoMap *map, const uint32 hash, const STITypeInfo *type_info) {\n");
    fprintf(output, "    const STITypeInfo **slot = DM_insert(map, hash);\n");
    fprintf(output, "    *slot = type_info;\n");
    fprintf(output, "}\n\n");
    fprintf(output, "STITypeInfoMap %s_type_info;\n", String_cstr(namespace));

    fprintf(output, "void STI_%s_register_functions(){\n", String_cstr(namespace));
    fprintf(output, "    DM_init(&ADF_TYPES_type_info, STITypeInfo, %u);\n", DM_count(&lib->types));
    for (int i = 0; i < DM_count(&lib->types); ++i) {
        const STI_Type *type = DM_get_value(&lib->types, i);
        if (type->type == STI_InlineArray ||
            type->type == STI_Alias ||
            type->type == STI_Bitfield ||
            type->type == STI_Pointer ||
            type->type == STI_DeferredType
        ) {
            continue;
        }
        fprintf(output, "    register_type_info(&%s_type_info, 0x%08X, &%s_TI);\n", String_cstr(namespace), type->hash,
                String_cstr(&type->name));
    }
    fprintf(output, "}\n\n");
}

void STI_generate_struct_forward_declaration(const STI_Type *type, FILE *output) {
    if (type->type != STI_Structure) {
        return;
    }

    const String *type_name = &type->name;
    fprintf(output, "typedef struct %s %s;// size: %i\n", String_cstr(type_name), String_cstr(type_name),
            type->size);
}

void forward_declare_functions(const STI_TypeLibrary *lib, const STI_Type *type, FILE *out) {
    if (type->type == STI_InlineArray ||
        type->type == STI_Pointer ||
        type->type == STI_Primitive ||
        type->type == STI_Alias ||
        type->type == STI_StringHash ||
        type->type == STI_Bitfield ||
        type->type == STI_DeferredType
    ) {
        return;
    }

    const bool is_complex = is_complex_type(lib, type);
    const bool need_init = is_complex || type->type == STI_Array || type->type == STI_Structure;

    if (need_init) {
        fprintf(out, "void %s_init(%s *obj);\n", String_cstr(&type->name), String_cstr(&type->name));
    }
    fprintf(out, "bool %s_read(%s *obj, Buffer* buffer);\n", String_cstr(&type->name), String_cstr(&type->name));
    fprintf(out, "void %s_print(%s *obj, JsonContext* ctx);\n", String_cstr(&type->name),
            String_cstr(&type->name));
    if (is_complex) {
        fprintf(out, "void %s_free(%s *obj);\n", String_cstr(&type->name), String_cstr(&type->name));
    }
}

void generate_type_info(const STI_TypeLibrary *lib, const STI_Type *type, FILE *out) {
    if (type->type == STI_InlineArray ||
        type->type == STI_Alias ||
        type->type == STI_Bitfield ||
        type->type == STI_Pointer ||
        type->type == STI_DeferredType
    ) {
        return;
    }
    // typedef struct STITypeInfo {
    //     initSTIObject init;
    //     readSTIObject read;
    //     freeSTIObject free;
    //     printSTIObject print;
    //     uint32 size;
    //     uint32 disk_size:30;
    //     uint32 is_struct:1;
    //     uint32 is_array:1;
    //     uint32 hash;
    //     const char* name;
    // }STITypeInfo;
    fprintf(out, "STITypeInfo %s_TI = {\n", String_cstr(&type->name));
    if (is_complex_type(lib, type)) {
        fprintf(out, "    .init = (initSTIObject)%s_init,\n", String_cstr(&type->name));
    }
    else {
        fprintf(out, "    .init = NULL,\n");
    }
    fprintf(out, "    .read = (readSTIObject)%s_read,\n", String_cstr(&type->name));
    if (is_complex_type(lib, type)) {
        fprintf(out, "    .free = (freeSTIObject)%s_free,\n", String_cstr(&type->name));
    }
    else {
        fprintf(out, "    .free = NULL,\n");
    }
    fprintf(out, "    .print = (printSTIObject)%s_print,\n", String_cstr(&type->name));
    fprintf(out, "    .size = sizeof(%s),\n", String_cstr(&type->name));
    fprintf(out, "    .disk_size = %i,\n", type->size);
    fprintf(out, "    .is_struct = %i,\n", type->type == STI_Structure ? 1 : 0);
    fprintf(out, "    .is_array = %i,\n", type->type == STI_Array ? 1 : 0);
    fprintf(out, "    .hash = 0x%08X,\n", type->hash);
    fprintf(out, "    .name = \"%s\"\n", String_cstr(&type->name));
    fprintf(out, "};\n\n");
}

void STI_TypeLibrary_generate_types(STI_TypeLibrary *lib, const String *namespace, FILE *header_output,
                                    const String *relative_header_path, FILE *impl_output) {
    fprintf(header_output, "// This file is autogenerated\n");
    fprintf(header_output, "#ifndef %s_GUARD\n", String_cstr(namespace));
    fprintf(header_output, "#define %s_GUARD\n\n", String_cstr(namespace));
    fprintf(header_output, "#include \"apex/adf/sti.h\"\n");
    fprintf(header_output, "#include \"apex/adf/sti_shared.h\"\n");
    fprintf(header_output, "#include \"apex/adf/adf_type_info_map.h\"\n");

    fprintf(header_output, "void STI_%s_register_functions();\n\n",
            String_cstr(namespace));


    fprintf(impl_output, "// This file is autogenerated\n");
    fprintf(impl_output, "#include \"%s\"\n\n", String_cstr(relative_header_path));
    fprintf(impl_output, "#include \"apex/adf/adf_support_types.h\"\n");
    fprintf(impl_output, "#include \"utils/dynamic_map.h\"\n");
    fprintf(impl_output, "#include \"utils/memory_profiling.h\"\n");
    fprintf(impl_output, "#include \"utils/json.h\"\n");
    fprintf(impl_output, "#include <assert.h>\n");

    for (int32 i = 0; i < STI_TypeLibrary_types_count(lib); i++) {
        const STI_Type *type = DM_get_value(&lib->types, i);
        STI_generate_struct_forward_declaration(type, header_output);
    }
    fprintf(header_output, "\n");
    for (int32 i = 0; i < STI_TypeLibrary_types_count(lib); i++) {
        const STI_Type *type = DM_get_value(&lib->types, i);
        STI_dump_type(lib, type, header_output);
    }
    fprintf(header_output, "\n");

    for (int32 i = 0; i < STI_TypeLibrary_types_count(lib); i++) {
        const STI_Type *type = DM_get_value(&lib->types, i);
        forward_declare_functions(lib, type, impl_output);
        generate_type_info(lib, type, impl_output);
    }
    fprintf(impl_output, "\n\n");

    for (int32 i = 0; i < STI_TypeLibrary_types_count(lib); i++) {
        const STI_Type *type = DM_get_value(&lib->types, i);
        STI_generate_init_function(lib, type, impl_output);
        STI_generate_read_function(lib, type, impl_output);
        STI_generate_free_function(lib, type, impl_output);
        STI_generate_print_function(lib, type, impl_output);
    }
    STI_generate_register_function(lib, namespace, impl_output);
    fprintf(header_output, "extern STITypeInfoMap %s_type_info;\n\n", String_cstr(namespace));

    fprintf(header_output, "#endif //%s_GUARD\n", String_cstr(namespace));
}
