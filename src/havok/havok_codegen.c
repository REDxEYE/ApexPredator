// Created by RED on 12.10.2025.

#include "havok/havok_codegen.h"
#include "havok/tag_file/havok_tag_types.h"
#include "utils/common.h"
#include "utils/hash_helper.h"


String *Havok_full_tag_type_name(const HKTagType *type) {
    String *full_name = String_new(16);
    if (type->name.buffer[0] == 'T' && type->name.buffer[1] == '*') {
        assert(type->template_args.count==1);
        String_append_str(full_name, &type->template_args.items[0].type->name);
        String_append_cstr(full_name, "*");
        return full_name;
    }
    String_append_str(full_name, &type->name);
    DA_FORI(type->template_args, i) {
        const HKTagTemplateArgument *arg = &type->template_args.items[i];
        if (arg->is_class) {
            assert(arg->type != NULL);
            String_append_cstr(full_name, "_");
            String_append_str(full_name, &arg->type->name);
        } else if (arg->is_number) {
            String_append_format(full_name, "_%u", arg->number);
        } else {
            String_append_cstr(full_name, "_");
            String_append_str(full_name, &arg->name);
        }
    }
    return full_name;
}

String *Havok_full_type_name(const HavokTypeLib *lib, const HavokType *type) {
    String *full_name = String_new(16);
    if (type->is_enum) {
        if (type->template_arguments.count > 0) {
            const HavokType *inner_type = DM_get(&lib->types, type->template_arguments.items[0].type_hash);
            const HavokType *storage_type = DM_get(&lib->types, type->template_arguments.items[1].type_hash);
            assert(inner_type!=NULL);
            assert(storage_type!=NULL);
            String_append_str(full_name, &inner_type->name);
            String_append_cstr(full_name, "_");
            String_append_str(full_name, &storage_type->name);
        } else {
            String_append_str(full_name, &type->name);
        }
        return full_name;
    }
    if (type->is_fixed_array && type->name.buffer[0] == 'T' && type->name.buffer[1] == '[') {
        assert(type->template_arguments.count==2);
        const HavokType *inner_type = DM_get(&lib->types, type->template_arguments.items[0].type_hash);
        const HavokTemplateArgument *size_arg = &type->template_arguments.items[1];
        String *inner_type_name = Havok_full_type_name(lib, inner_type);
        assert(inner_type!=NULL);
        String_append_cstr(full_name, "Array_");
        if (size_arg->is_number) {
            String_append_format(full_name, "%u", size_arg->number);
        } else {
            String_append_str(full_name, &size_arg->name);
        }
        String_append_cstr(full_name, "_");
        String_append_str(full_name, inner_type_name);
        String_free(inner_type_name);
        return full_name;
    }

    if (type->name.buffer[0] == 'T' && type->name.buffer[1] == '*') {
        assert(type->template_arguments.count==1);
        const HavokType *inner_type = DM_get(&lib->types, type->template_arguments.items[0].type_hash);
        String *inner_type_name = Havok_full_type_name(lib, inner_type);
        String_append_str(full_name, inner_type_name);
        String_append_cstr(full_name, "*");
        String_free(inner_type_name);
        return full_name;
    }
    String_append_str(full_name, &type->name);
    DA_FORI(type->template_arguments, i) {
        const HavokTemplateArgument *arg = DA_at(&type->template_arguments, i);
        if (arg->is_class) {
            String_append_cstr(full_name, "_");
            const HavokType *inner_type = DM_get(&lib->types, arg->type_hash);
            assert(inner_type!=NULL);
            String_append_str(full_name, &inner_type->name);
        } else if (arg->is_number) {
            String_append_format(full_name, "_%u", arg->number);
        } else {
            String_append_cstr(full_name, "_");
            String_append_str(full_name, &arg->name);
        }
    }
    return full_name;
}

void HavokTemplateArgument_free(HavokTemplateArgument *arg) {
    String_free(&arg->name);
    arg->type_hash = 0;
    arg->number = 0;
    arg->is_number = 0;
    arg->is_class = 0;
}

void HavokRecordMember_free(HavokRecordMember *member) {
    String_free(&member->name);
    member->type_hash = 0;
    member->flags = 0;
    member->offset = 0;
}

void HavokType_init(HavokType *type) {
    String_init(&type->name, 32);
    type->hash = 0;
    type->parent_hash = 0;
    type->size = 0;
    type->align = 0;
    DA_init(&type->template_arguments, HavokTemplateArgument, 4);
    DA_init(&type->members, HavokRecordMember, 8);
}

void HavokType_free(HavokType *type) {
    String_free(&type->name);
    DA_free_with_inner(&type->template_arguments, {HavokTemplateArgument_free(it);});
    DA_free_with_inner(&type->members, {HavokRecordMember_free(it);});
}

void register_alias(HavokTypeLib *lib, const char *havok_name, const char *real_name) {
    uint32 havok_type_hash = hash_cstring(havok_name);
    HavokType *havok_type = DM_get(&lib->types, havok_type_hash);
    if (havok_type == NULL) {
        printf("[WARN]: Cannot register alias %s for unknown type %s\n", real_name, havok_name);
        return;
    }
    if (havok_type->parent_hash != 0) {
        printf("[WARN]: Cannot register alias %s for type %s with parent\n", real_name, havok_name);
        return;
    }

    uint32 real_type_hash = hash_cstring(real_name);
    HavokType *real_type = DM_get(&lib->types, real_type_hash);
    if (real_type != NULL) {
        printf("[WARN]: Cannot register alias %s for already existing type %s\n", real_name, havok_name);
        return;
    }
    havok_type->parent_hash = real_type_hash;
    HavokType *new_type = DM_insert(&lib->types, real_type_hash);
    HavokType_init(new_type);
    String_from_cstr(&new_type->name, real_name);
    new_type->hash = real_type_hash;
    new_type->size = havok_type->size;
    new_type->align = havok_type->align;
    new_type->is_primitive = true;
}

void HavokTypeLib_init(HavokTypeLib *lib) {
    DM_init(&lib->types, HavokType, 1024);
    DA_init(&lib->exported_hashes, uint64, 1024);
    DM_init(&lib->object_functions, HAVOK_ObjectMethods, 1024);
}

void HavokTypeLib_free(HavokTypeLib *lib) {
    for (int i = 0; i < lib->types.values.count; ++i) {
        HavokType *type = DA_at(&lib->types.values, i);
        HavokType_free(type);
    }
    DM_free(&lib->types);
    DA_free(&lib->exported_hashes);
}

HavokType *HavokTypeLib_find_by_name(HavokTypeLib *lib, const char *name) {
    uint64 type_hash = hash_cstring(name);
    return DM_get(&lib->types, type_hash);
}

HavokType *HavokTypeLib__register_type(HavokTypeLib *lib, const HKTagType *tf_type);

HavokType *HavokTypeLib__register_type(HavokTypeLib *lib, const HKTagType *tf_type) {
    String *full_tf_type_name = Havok_full_tag_type_name(tf_type);
    uint64 type_hash = hash_string(full_tf_type_name);
    if (DA_contains(&lib->exported_hashes, &type_hash, compare_hashes64)) {
        String_free(full_tf_type_name);
        return DM_get(&lib->types, type_hash);
    }

    if (tf_type->parent != NULL) {
        HavokTypeLib__register_type(lib, tf_type->parent);
    }


    DA_FORI(tf_type->template_args, i) {
        const HKTagTemplateArgument *tf_arg = &tf_type->template_args.items[i];
        if (tf_arg->is_class) {
            assert(tf_arg->type != NULL);
            HavokTypeLib__register_type(lib, tf_arg->type);
        }
    }

    HavokType *existing_type = DM_get(&lib->types, type_hash);
    if (existing_type != NULL) {
        String *existing_full_name = Havok_full_type_name(lib, existing_type);
        if (!String_equals(existing_full_name, full_tf_type_name)) {
            printf("[WARN]: Duplicate type hash for type %s/%s\n", String_data(full_tf_type_name),
                   String_data(existing_full_name));
            String_free(full_tf_type_name);
            String_free(existing_full_name);
            exit(1);
        }
        String_free(full_tf_type_name);
        String_free(existing_full_name);
        return existing_type;
    }
    String_free(full_tf_type_name);

    if (tf_type->data_type == HKTYPE_POINTER && tf_type->name.buffer[0] == 'T') {
        assert(tf_type->members.count==0 && "Pointer type cannot have members");
    }

    DA_FORI(tf_type->members, j) {
        const HKTagTypeMember *tf_member = &tf_type->members.items[j];
        if (tf_member->type != NULL) {
            HavokTypeLib__register_type(lib, tf_member->type);
        }
    }
    bool is_enum = false;
    if (String_cequals(&tf_type->name, "hkEnum")) {
        is_enum = true;
        // Enums first template arg is it's actual type. It needs to be processed and treated as enum;
        const HKTagTemplateArgument *enum_type_arg = &tf_type->template_args.items[0];
        HavokType *inner_enum_type = HavokTypeLib__register_type(lib, enum_type_arg->type);
        inner_enum_type->is_enum = true;
    }

    HavokType *new_type = DM_insert(&lib->types, type_hash);
    HavokType_init(new_type);
    String_copy_from(&new_type->name, &tf_type->name);
    new_type->hash = type_hash;
    new_type->size = tf_type->size;
    new_type->align = tf_type->align;
    new_type->is_enum = is_enum;

    if (tf_type->members.count > 0) {
        new_type->is_record = 1;
    } else if (tf_type->data_type == HKTYPE_POINTER) {
        new_type->is_ptr = 1;
    } else if (tf_type->data_type == HKTYPE_ARRAY) {
        if (String_cequals(&tf_type->name, "T[N]")) {
            new_type->is_fixed_array = 1;
        } else {
            new_type->is_array = 1;
        }
    } else {
        new_type->is_primitive = 1;
    }
    if (tf_type->parent != NULL) {
        String *parent_full_name = Havok_full_tag_type_name(tf_type->parent);
        new_type->parent_hash = hash_string(parent_full_name);
        String_free(parent_full_name);
    }
    DA_reserve(&new_type->template_arguments, tf_type->template_args.count);
    DA_FORI(tf_type->template_args, j) {
        const HKTagTemplateArgument *tf_arg = &tf_type->template_args.items[j];
        HavokTemplateArgument *new_arg = DA_append_get(&new_type->template_arguments);
        String_copy_from(&new_arg->name, &tf_arg->name);
        new_arg->is_class = tf_arg->is_class;
        new_arg->is_number = tf_arg->is_number;
        new_arg->number = tf_arg->number;
        if (tf_arg->is_class) {
            assert(tf_arg->type != NULL);
            String *arg_full_name = Havok_full_tag_type_name(tf_arg->type);
            new_arg->type_hash = hash_string(arg_full_name);
            String_free(arg_full_name);
        }
    }
    DA_FORI(tf_type->members, j) {
        const HKTagTypeMember *tf_member = &tf_type->members.items[j];
        HavokRecordMember *new_member = DA_append_get(&new_type->members);
        String_copy_from(&new_member->name, &tf_member->name);
        if (String_cequals(&new_member->name, "bool")) {
            String_from_cstr(&new_member->name, "bool__");
        }
        new_member->flags = tf_member->flags;
        new_member->offset = tf_member->offset;
        if (tf_member->type != NULL) {
            String *member_type_full_name = Havok_full_tag_type_name(tf_member->type);
            new_member->type_hash = hash_string(member_type_full_name);
            String_free(member_type_full_name);
        } else {
            printf("[ERROR]: Member %s has NULL type\n", String_data(&tf_member->name));
        }
    }

    if (new_type->parent_hash != 0) {
        const HavokType *parent_type = DM_get(&lib->types, new_type->parent_hash);
        if (new_type->size == 0) {
            new_type->size = parent_type->size;
        }
        if (new_type->align == 0) {
            new_type->align = parent_type->align;
        }
    }

    DA_append(&lib->exported_hashes, &type_hash);
    return new_type;
}

void HavokTypeLib_copy_from_tag_file(HavokTypeLib *lib, TagFile *tf) {
    DA_FORI(tf->types, i) {
        if (i == 0)continue;
        const HKTagType *tf_type = DA_at(&tf->types, i);
        HavokTypeLib__register_type(lib, tf_type);
    }
}

static uint32 indent = 0;

void generate_members(const HavokTypeLib *lib, const HavokType *record_type, FILE *header_output,
                      uint32 *prev_member_offset, uint32 *prev_member_size) {
    if (record_type->parent_hash != 0) {
        const HavokType *parent_type = DM_get(&lib->types, record_type->parent_hash);
        generate_members(lib, parent_type, header_output, prev_member_offset, prev_member_size);
    }

    const String *full_name = Havok_full_type_name(lib, record_type);
    fprintf(header_output, "    // %s members\n", String_data(full_name));
    DA_FORI(record_type->members, i) {
        const HavokRecordMember *member = &record_type->members.items[i];
        const HavokType *member_type = DM_get(&lib->types, member->type_hash);
        if (member_type == NULL) {
            printf("No member type data: %s for member %s of type %s\n",
                   String_data(&member->name), String_data(full_name), String_data(&record_type->name));
            exit(1);
        }

        if (member->offset != 0) {
            // Compute inter padding
            const uint32 expected_offset = *prev_member_offset + *prev_member_size;
            if (member->offset > expected_offset) {
                const uint32 padding_size = member->offset - expected_offset;
                fprintf(header_output, "    uint8 _padding_%s[%d]; // inter-member padding\n",
                        String_data(&member->name), padding_size);
            }
        }

        String *member_full_name = Havok_full_type_name(lib, member_type);
        if (member_type->is_array) {
            if (member_type->template_arguments.count != 0) {
                printf("[ERROR]: Unsupported dynamic array member type for member %s of type %s\n",
                       String_data(&member->name), String_data(full_name));
                exit(1);
            }
            fprintf(header_output, "    %s %s; // offset: %d, flags: %d, size: %d\n",
                    String_data(member_full_name),
                    String_data(&member->name), member->offset, member->flags,
                    member_type->size);
        } else if (member_type->is_fixed_array) {
            const HavokType *inner_type = DM_get(&lib->types, member_type->template_arguments.items[0].type_hash);
            String *inner_type_name = Havok_full_type_name(lib, inner_type);
            fprintf(header_output, "    %s %s[%i]; // offset: %d, flags: %d, size: %d, hash: 0x%08X\n",
                    String_data(inner_type_name),
                    String_data(&member->name), member_type->template_arguments.items[1].number,
                    member->offset, member->flags,
                    member_type->size, member_type->hash);
            String_free(inner_type_name);
        } else {
            fprintf(header_output, "    %s %s; // offset: %d, flags: %d, size: %d\n",
                    String_data(member_full_name),
                    String_data(&member->name), member->offset, member->flags,
                    member_type->size);
        }
        String_free(member_full_name);

        *prev_member_offset = member->offset;
        *prev_member_size = member_type->size;
    }
}

void generate_type_def(const HavokTypeLib *lib, const HavokType *type, FILE *header_output) {
    if (DA_contains(&lib->exported_hashes, &type->hash, compare_hashes64)) {
        return;
    }
    DA_append(&lib->exported_hashes, &type->hash);

    String *full_name = Havok_full_type_name(lib, type);
    // Print indent
    // for (uint32 j = 0; j < indent; ++j) {
    //     printf("  ");
    // }
    // printf("Generating %s\n", String_data(full_name));


    String safe_type_name = {0};
    String_copy_from(&safe_type_name, full_name);
    if (safe_type_name.buffer[safe_type_name.size - 1] == '*') {
        safe_type_name.size--;
        String_append_cstr(&safe_type_name, "_Ptr");
    }

    if (type->is_record) {
        if (type->parent_hash != 0) {
            const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
            if (parent_type != NULL) {
                indent++;
                generate_type_def(lib, parent_type, header_output);
                indent--;
            }
        }

        DA_FORI(type->members, i) {
            const HavokRecordMember *member = &type->members.items[i];
            const HavokType *member_type = DM_get(&lib->types, member->type_hash);
            if (member_type != NULL) {
                indent++;
                generate_type_def(lib, member_type, header_output);
                indent--;
            }
        }


        fprintf(header_output, "#define %s_HASH 0x%08X\n", String_data(&safe_type_name), type->hash);
        fprintf(header_output, "typedef /*alignas(%i)*/ struct %s {\n", type->align, String_data(&safe_type_name));
        uint32 prev_offset, prev_size;
        generate_members(lib, type, header_output, &prev_offset, &prev_size);

        // Final padding
        const uint32 expected_size = prev_offset + prev_size;
        if (expected_size < type->size) {
            const uint32 padding_size = type->size - expected_size;
            fprintf(header_output, "    uint8 _padding_end[%d]; // final padding\n", padding_size);
        }

        fprintf(header_output, "} %s;\n", String_data(full_name));
        fprintf(header_output, "static_assert(sizeof(%s)==%i, \"Invalid size for %s\");\n\n",
                String_data(&safe_type_name), type->size, String_data(&safe_type_name));
        String_free(&safe_type_name);
        String_free(full_name);
        return;
    }

    if (type->is_enum) {
        assert(type->members.count==0);
        if (type->template_arguments.count == 2) {
            const HavokType *enum_type = DM_get(&lib->types, type->template_arguments.items[0].type_hash);
            const HavokType *container_type = DM_get(&lib->types, type->template_arguments.items[1].type_hash);
            assert(enum_type!=NULL);
            assert(container_type!=NULL);
            indent++;
            generate_type_def(lib, enum_type, header_output);
            generate_type_def(lib, container_type, header_output);
            indent--;
            fprintf(header_output, "#define %s_HASH 0x%08X\n", String_data(&safe_type_name), type->hash);
            fprintf(header_output, "typedef %s %s;\n\n", String_data(&container_type->name),
                    String_data(&safe_type_name));
        } else {
            switch (type->size) {
                case 1: {
                    fprintf(header_output, "#define %s_HASH 0x%08X\n", String_data(&safe_type_name), type->hash);
                    fprintf(header_output, "typedef uint8 %s;\n\n", String_data(&safe_type_name));
                    break;
                }
                case 2: {
                    fprintf(header_output, "#define %s_HASH 0x%08X\n", String_data(&safe_type_name), type->hash);
                    fprintf(header_output, "typedef uint16 %s;\n\n", String_data(&safe_type_name));
                    break;
                }
                case 4: {
                    fprintf(header_output, "#define %s_HASH 0x%08X\n", String_data(&safe_type_name), type->hash);
                    fprintf(header_output, "typedef uint32 %s;\n\n", String_data(&safe_type_name));
                    break;
                }
                default: {
                    printf("[ERROR]: Unsupported enum size %d for type %s\n", type->size, String_data(full_name));
                    exit(1);
                }
            }
        }
        String_free(full_name);
        return;
    }
    if (type->is_primitive) {
        if (type->parent_hash == 0) {
            fprintf(header_output, "/* primitive %s 0x%08X\n", String_data(&safe_type_name), type->hash);
            fprintf(header_output, "size: %i alignment: %i template args: %i */\n", type->size, type->align,
                    type->template_arguments.count);
            return;
        }
        const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
        indent++;
        generate_type_def(lib, parent_type, header_output);
        indent--;
        String *parent_full_name = Havok_full_type_name(lib, parent_type);
        fprintf(header_output, "#define %s_HASH 0x%08X\n", String_data(&safe_type_name), type->hash);
        fprintf(header_output, "typedef %s %s; // size: %i alignment %i \n\n", String_data(parent_full_name),
                String_data(full_name), type->size, type->align);
        String_free(parent_full_name);
        String_free(full_name);
        return;
    }
    if (type->is_ptr) {
        if (type->template_arguments.count != 1) {
            printf("[ERROR]: Unsupported pointer type %s without template argument\n", String_data(full_name));
            exit(1);
        }
        const HavokTemplateArgument *inner_type_arg = &type->template_arguments.items[0];
        assert(inner_type_arg->is_class);
        const HavokType *inner_type = DM_get(&lib->types, inner_type_arg->type_hash);
        assert(inner_type!=NULL);
        indent++;
        generate_type_def(lib, inner_type, header_output);
        indent--;
        String_free(full_name);
        return;
    }

    if (type->is_array || String_cequals(&type->name, "hkRotationImpl")

    ) {
        // Very special case for now
        if (type->template_arguments.count != 1) {
            printf("[ERROR]: Unsupported array type %s without template argument\n", String_data(full_name));
            exit(1);
        }
        const HavokTemplateArgument *inner_type_arg = &type->template_arguments.items[0];
        assert(inner_type_arg->is_class);
        const HavokType *inner_type = DM_get(&lib->types, inner_type_arg->type_hash);
        assert(inner_type!=NULL);
        indent++;
        generate_type_def(lib, inner_type, header_output);
        indent--;
        uint32 item_count = type->size / inner_type->size;
        String *inner_full_name = Havok_full_type_name(lib, inner_type);
        fprintf(header_output, "#define %s_HASH 0x%08X\n", String_data(full_name), type->hash);
        fprintf(header_output, "typedef %s %s[%u];\n\n", String_data(inner_full_name), String_data(full_name),
                item_count);
        String_free(inner_full_name);
        String_free(full_name);
        return;
    }
    if (type->is_fixed_array) {
        const HavokTemplateArgument *inner_type_arg = &type->template_arguments.items[0];
        const HavokType *inner_type = DM_get(&lib->types, inner_type_arg->type_hash);
        indent++;
        generate_type_def(lib, inner_type, header_output);
        indent--;
        String_free(full_name);
        return;
    }

    printf("[WARN]: Skipping non-struct type %s\n", String_data(full_name));

    String_free(full_name);
}

void generate_function_forward_defs(const HavokTypeLib *lib, const HavokType *type, FILE *header_output) {
    if (DA_contains(&lib->exported_hashes, &type->hash, compare_hashes64)) {
        return;
    }
    DA_append(&lib->exported_hashes, &type->hash);

    String *full_name = Havok_full_type_name(lib, type);
    if (type->is_record) {
        fprintf(header_output, "void %s_read(const TagFile *tf, const HavokTypeLib* lib, %s *obj, const uint8* src);\n",
                String_data(full_name),
                String_data(full_name));
        fprintf(header_output, "void %s_print(const %s *obj, const HavokTypeLib* lib, JsonContext *ctx);\n", String_data(full_name), String_data(full_name));
        fprintf(header_output, "void %s_free(%s *obj);\n", String_data(full_name), String_data(full_name));
        fprintf(header_output, "\n");
    } else if (type->is_primitive) {
        if (type->parent_hash != 0) {
            const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
            generate_function_forward_defs(lib, parent_type, header_output);
            String *parent_full_name = Havok_full_type_name(lib, parent_type);
            fprintf(header_output,
                    "void %s_read(const TagFile *tf, const HavokTypeLib* lib, %s *obj, const uint8* src);\n",
                    String_data(full_name),
                    String_data(full_name));
            fprintf(header_output, "void %s_print(const %s *obj, const HavokTypeLib* lib, JsonContext *ctx);\n", String_data(full_name), String_data(full_name));
            String_free(parent_full_name);
        }
    } else if (String_cequals(&type->name, "hkRotationImpl")) {
        const HavokTemplateArgument *inner_type_arg = &type->template_arguments.items[0];
        assert(inner_type_arg->is_class);
        const HavokType *inner_type = DM_get(&lib->types, inner_type_arg->type_hash);
        String* inner_full_name = Havok_full_type_name(lib, inner_type);
        assert(inner_type!=NULL);
        generate_function_forward_defs(lib, inner_type, header_output);
        fprintf(header_output, "void %s_read(const TagFile *tf, const HavokTypeLib* lib, %s *obj, const uint8* src);\n",
                String_data(full_name),
                String_data(full_name));
        fprintf(header_output, "void %s_print(const %s *obj, const HavokTypeLib* lib, JsonContext *ctx);\n", String_data(full_name), String_data(full_name));
        String_free(inner_full_name);
    } else if (type->is_fixed_array) {
        const HavokTemplateArgument *inner_type_arg = &type->template_arguments.items[0];
        assert(inner_type_arg->is_class);
        const HavokType *inner_type = DM_get(&lib->types, inner_type_arg->type_hash);
        assert(inner_type!=NULL);
        generate_function_forward_defs(lib, inner_type, header_output);
        String *inner_full_name = Havok_full_type_name(lib, inner_type);
        fprintf(header_output, "void %s_read(const TagFile *tf, const HavokTypeLib* lib, %s *obj, const uint8* src);\n",
                String_data(full_name),
                String_data(inner_full_name));
        fprintf(header_output, "void %s_print(const %s *obj, const HavokTypeLib* lib, JsonContext *ctx);\n", String_data(full_name), String_data(inner_full_name));
        String_free(inner_full_name);
    }
    String_free(full_name);
}

void generate_obj_read_call(const HavokRecordMember *member, const HavokTypeLib *lib, const HavokType *type,
                            FILE *impl_output) {
    String *member_full_name = Havok_full_type_name(lib, type);
    fprintf(impl_output, "    %s_read(tf, lib, &obj->%s, src + %i); // Simple read\n", String_data(member_full_name),
            String_data(&member->name), member->offset);
    String_free(member_full_name);
}

void generate_ptr_read_call(const HavokRecordMember *member, const HavokTypeLib *lib, const HavokType *type,
                            FILE *impl_output) {
    const HavokType *inner_type = DM_get(&lib->types, type->template_arguments.items[0].type_hash);
    String *inner_full_name = Havok_full_type_name(lib, inner_type);
    if (inner_full_name->buffer[inner_full_name->size - 1] == '*') {
        inner_full_name->size--;
        String_append_cstr(inner_full_name, "_Ptr");
    }

    fprintf(impl_output, "    read_ptr(tf, lib, (void*)&obj->%s, src + %i, NULL);\n",
            String_data(&member->name), member->offset);
    String_free(inner_full_name);
}

void generate_fixed_array_read(const HavokRecordMember *member, const HavokTypeLib *lib, const HavokType *type,
                               FILE *impl_output) {
    String *member_full_name = Havok_full_type_name(lib, type);
    fprintf(impl_output, "    %s_read(tf, lib, obj->%s, src + %i); // Fixed array read\n",
            String_data(member_full_name),
            String_data(&member->name), member->offset);
    String_free(member_full_name);
}

void generate_read_function_body(const HavokTypeLib *lib, const HavokType *type, FILE *impl_output) {
    String *full_name = Havok_full_type_name(lib, type);
    if (type->is_record) {
        fprintf(impl_output, "void %s_read(const TagFile *tf, const HavokTypeLib* lib, %s *obj, const uint8* src) {\n",
                String_data(full_name),
                String_data(full_name));

        if (strcmp(String_data(&type->name), "hkArray")==0) {
            fprintf(impl_output, "    hkArray_read(tf, lib, obj, src);\n");
            fprintf(impl_output, "}\n");
            return;
        }

        if (type->parent_hash != 0) {
            const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
            String *parent_full_name = Havok_full_type_name(lib, parent_type);
            fprintf(impl_output, "    %s_read(tf, lib, (%s*)obj, src);\n", String_data(parent_full_name),
                    String_data(parent_full_name));
            String_free(parent_full_name);
        }

        DA_FORI(type->members, i) {
            const HavokRecordMember *member = &type->members.items[i];
            const HavokType *member_type = DM_get(&lib->types, member->type_hash);
            String *member_type_name = Havok_full_type_name(lib, member_type);
            if (!member_type->is_ptr && member_type_name->buffer[member_type_name->size - 1] == '*') {
                fprintf(impl_output, "    exit(1); // Unimplemented case %s %s\n", String_data(member_type_name),
                        String_data(&member->name));
                String_free(member_type_name);
                continue;
            }
            String_free(member_type_name);

            if (member_type->is_ptr) {
                generate_ptr_read_call(member, lib, member_type, impl_output);
            } else if (member_type->is_fixed_array) {
                generate_fixed_array_read(member, lib, member_type, impl_output);
            } else {
                generate_obj_read_call(member, lib, member_type, impl_output);
            }
        }
        fprintf(impl_output, "}\n");
    } else if (type->is_fixed_array) {
        const HavokTemplateArgument *inner_type_arg = &type->template_arguments.items[0];
        assert(inner_type_arg->is_class);
        const HavokType *inner_type = DM_get(&lib->types, inner_type_arg->type_hash);
        assert(inner_type!=NULL);
        String *inner_full_name = Havok_full_type_name(lib, inner_type);
        fprintf(impl_output, "void %s_read(const TagFile *tf, const HavokTypeLib* lib, %s *obj, const uint8* src) {\n",
                String_data(full_name), String_data(inner_full_name));
        fprintf(impl_output, "    memcpy(obj, src, %i);\n", type->size);
        fprintf(impl_output, "}\n");
        String_free(inner_full_name);
    } else if (String_cequals(&type->name, "hkRotationImpl")) {
        const HavokTemplateArgument *inner_type_arg = &type->template_arguments.items[0];
        assert(inner_type_arg->is_class);
        const HavokType *inner_type = DM_get(&lib->types, inner_type_arg->type_hash);
        assert(inner_type!=NULL);
        generate_function_forward_defs(lib, inner_type, impl_output);
        const uint32 item_count = type->size / inner_type->size;
        String *inner_full_name = Havok_full_type_name(lib, inner_type);
        fprintf(impl_output, "void %s_read(const TagFile *tf, const HavokTypeLib* lib, %s *obj, const uint8* src) {\n",
                String_data(full_name),
                String_data(full_name));
        fprintf(impl_output, "    for (uint32 i = 0; i < %u; ++i) {\n", item_count);
        fprintf(impl_output, "        %s_read(tf, lib, &(*obj)[i], src + i * %u);\n", String_data(inner_full_name),
                inner_type->size);
        fprintf(impl_output, "    }\n");
        fprintf(impl_output, "}\n");
        String_free(inner_full_name);
    } else if (type->is_primitive) {
        if (type->parent_hash == 0) {
            String_free(full_name);
            return;
        }
        const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
        String *parent_full_name = Havok_full_type_name(lib, parent_type);
        fprintf(impl_output, "void %s_read(const TagFile *tf, const HavokTypeLib* lib, %s *obj, const uint8* src) {\n",
                String_data(full_name),
                String_data(full_name));
        fprintf(impl_output, "    %s_read(tf, lib, (%s*)obj, src);\n", String_data(parent_full_name),
                String_data(parent_full_name));
        fprintf(impl_output, "}\n");
        String_free(parent_full_name);
    }
}

void generate_print_function_body(const HavokTypeLib *lib, const HavokType *type, FILE *impl_output);

void generate_type_print_body(const HavokTypeLib *lib, const HavokType *type, FILE *impl_output) {
    if (type->is_record) {
        if (type->name.size>13 && strcmp(String_data(&type->name)+type->name.size-13, "_NamedVariant")==0) {
            fprintf(impl_output, "    NamedVariant_print(obj, lib, ctx);\n");
            return;
        }

        if (type->parent_hash != 0) {
            const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
            generate_type_print_body(lib, parent_type, impl_output);
        }
        DA_FORI(type->members, i) {
            const HavokRecordMember *member = &type->members.items[i];
            const HavokType *member_type = DM_get(&lib->types, member->type_hash);
            String *full_type_name = Havok_full_type_name(lib, member_type);
            fprintf(impl_output, "    jsonName(ctx, \"%s\");\n", String_data(&member->name));
            if (member_type->is_record) {
                fprintf(impl_output, "    %s_print(&obj->%s, lib, ctx);\n",
                        String_data(full_type_name),
                        String_data(&member->name));
            } else if (member_type->is_ptr) {
                const HavokType *inner_type = DM_get(&lib->types, member_type->template_arguments.items[0].type_hash);
                String *inner_full_name = Havok_full_type_name(lib, inner_type);
                if (inner_full_name->buffer[inner_full_name->size - 1] == '*') {
                    inner_full_name->size--;
                    String_append_cstr(inner_full_name, "_Ptr");
                }
                fprintf(impl_output, "    %s_print(obj->%s, lib, ctx);\n",
                        String_data(inner_full_name),
                        String_data(&member->name));
                String_free(inner_full_name);
            }else if (member_type->is_fixed_array){
                fprintf(impl_output, "    %s_print(obj->%s, lib, ctx);\n",
                        String_data(full_type_name),
                        String_data(&member->name));
            }else {
                fprintf(impl_output, "    %s_print(&obj->%s, lib, ctx);\n",
                        String_data(full_type_name),
                        String_data(&member->name));
            }
            String_free(full_type_name);
        }
    }
}

void generate_print_function_body(const HavokTypeLib *lib, const HavokType *type, FILE *impl_output) {
    String *full_type_name = Havok_full_type_name(lib, type);
    if (String_cequals(&type->name, "hkRotationImpl")) {
        const HavokTemplateArgument *inner_type_arg = &type->template_arguments.items[0];
        assert(inner_type_arg->is_class);
        const HavokType *inner_type = DM_get(&lib->types, inner_type_arg->type_hash);
        assert(inner_type!=NULL);
        String *inner_full_name = Havok_full_type_name(lib, inner_type);
        const uint32 item_count = type->size / inner_type->size;
        fprintf(impl_output, "void %s_print(const %s *obj, const HavokTypeLib* lib, JsonContext *ctx){\n", String_data(full_type_name),
                String_data(full_type_name));
        fprintf(impl_output, "    if(obj==NULL){jsonValueNull(ctx); return;}\n");
        fprintf(impl_output, "    jsonBeginCompactArray(ctx);\n");
        fprintf(impl_output, "    for (uint32 i = 0; i < %u; ++i) {\n", item_count);
        fprintf(impl_output, "        %s_print(&*obj[i], lib, ctx);\n", String_data(inner_full_name));
        fprintf(impl_output, "    }\n");
        fprintf(impl_output, "    jsonEndCompactArray(ctx);\n");
        fprintf(impl_output, "}\n\n");
        String_free(inner_full_name);
    }else if (type->is_record) {
        fprintf(impl_output, "void %s_print(const %s *obj, const HavokTypeLib* lib, JsonContext *ctx){\n", String_data(full_type_name), String_data(full_type_name));
        fprintf(impl_output, "    if(obj==NULL){jsonValueNull(ctx); return;}\n");
        if (strcmp(String_data(&type->name), "hkArray")==0) {
            const HavokType* inner_type = DM_get(&lib->types, type->template_arguments.items[0].type_hash);
            String* inner_type_name = Havok_full_type_name(lib, inner_type);
            fprintf(impl_output, "    hkArray_print(obj, lib, ctx, \"%s\");\n", String_data(inner_type_name));
            String_free(inner_type_name);
        }else {
            fprintf(impl_output, "    jsonBeginObject(ctx);\n");
            fprintf(impl_output, "    jsonNameValueStr(ctx, \"__class\", \"%s\");\n", String_data(full_type_name));
            generate_type_print_body(lib, type, impl_output);
            fprintf(impl_output, "    jsonEndObject(ctx);\n");
        }
        fprintf(impl_output, "}\n\n");
    } else if (type->is_primitive) {
        if (type->parent_hash == 0) {
            String_free(full_type_name);
            return;
        }
        fprintf(impl_output, "void %s_print(const %s *obj, const HavokTypeLib* lib, JsonContext *ctx){\n", String_data(full_type_name), String_data(full_type_name));
        fprintf(impl_output, "    if(obj==NULL){jsonValueNull(ctx); return;}\n");
        const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
        String *parent_type_name = Havok_full_type_name(lib, parent_type);
        fprintf(impl_output, "   %s_print(obj, lib, ctx);\n", String_data(parent_type_name));
        String_free(parent_type_name);
        fprintf(impl_output, "}\n\n");
    } else if (type->is_fixed_array) {
        const HavokTemplateArgument *inner_type_arg = &type->template_arguments.items[0];
        assert(inner_type_arg->is_class);
        const HavokType *inner_type = DM_get(&lib->types, inner_type_arg->type_hash);
        assert(inner_type!=NULL);
        String *inner_full_name = Havok_full_type_name(lib, inner_type);
        const uint32 item_count = type->template_arguments.items[0].number;
        fprintf(impl_output, "void %s_print(const %s *obj, const HavokTypeLib* lib, JsonContext *ctx){\n", String_data(full_type_name),
                String_data(inner_full_name));
        fprintf(impl_output, "    if(obj==NULL){jsonValueNull(ctx); return;}\n");
        fprintf(impl_output, "    jsonBeginCompactArray(ctx);\n");
        fprintf(impl_output, "    for (uint32 i = 0; i < %u; ++i) {\n", item_count);
        fprintf(impl_output, "        %s_print(&obj[i], lib, ctx);\n", String_data(inner_full_name));
        fprintf(impl_output, "    }\n");
        fprintf(impl_output, "    jsonEndCompactArray(ctx);\n");
        fprintf(impl_output, "}\n\n");
        String_free(inner_full_name);
    } else {
        printf("Unhandled type: %s\n", String_data(full_type_name));
    }
    String_free(full_type_name);
}

void generate_function_table(HavokTypeLib *lib, FILE *header_output, FILE *impl_output) {
    fprintf(header_output, "void HAVOK_TYPES_register_functions(HavokTypeLib* lib);\n");
    fprintf(impl_output, "void HAVOK_TYPES_register_functions(HavokTypeLib* lib){\n");
    DA_FORI(lib->types.values, i) {
        const HavokType *type = &lib->types.values.items[i];
        if (type->is_ptr || type->is_enum) {
            continue;
        }
        String *full_name = Havok_full_type_name(lib, type);
        String safe_type_name = {0};
        String_copy_from(&safe_type_name, full_name);
        if (safe_type_name.buffer[safe_type_name.size - 1] == '*') {
            safe_type_name.size--;
            String_append_cstr(&safe_type_name, "_Ptr");
        }
        fprintf(impl_output,
                // "*((HAVOK_ObjectMethods*)(DM_insert(&lib->object_functions, 0x%08X))) = (HAVOK_ObjectMethods){(readHavokObject)%s_read, (freeHavokObject)%s_free, (printHavokObject)%s_free} // is_ptr %i, is_record %i, is_primitive %i, is_enum %i\n;",
                "*((HAVOK_ObjectMethods*)(DM_insert(&lib->object_functions, 0x%08X))) = (HAVOK_ObjectMethods){(readHavokObject)%s_read, (freeHavokObject)/*%s_free*/NULL, (printHavokObject)%s_print}; // is_ptr %i, is_record %i, is_primitive %i, is_enum %i\n",
                type->hash,
                String_data(&safe_type_name),
                String_data(&safe_type_name),
                String_data(&safe_type_name),
                type->is_ptr,
                type->is_record,
                type->is_primitive,
                type->is_enum
        );
        String_free(full_name);
        String_free(&safe_type_name);
    }
    fprintf(impl_output, "}\n");
}

void HavokTypeLib_generate_code(HavokTypeLib *lib, const String *namespace, FILE *header_output,
                                const String *header_relative_path, FILE *impl_output) {
    DA_init(&lib->exported_hashes, uint64, lib->types.values.count);

    register_alias(lib, "hkVector4f", "HavokVector4");

    HavokType *tmp = HavokTypeLib_find_by_name(lib, "hkVector4f");
    HavokType *uint64_type = HavokTypeLib_find_by_name(lib, "unsigned_long_long");
    tmp->is_array = false;
    tmp->is_primitive = true;
    tmp = HavokTypeLib_find_by_name(lib, "hkBaseObject");
    tmp->is_record = true;
    tmp->is_primitive = false;
    HavokRecordMember fake_member = {0};
    String_from_cstr(&fake_member.name, "unk_member");
    fake_member.offset = 0;
    fake_member.flags = 0;
    fake_member.type_hash = uint64_type->hash;
    DA_append(&tmp->members, &fake_member);

    const HavokType *charPtr = HavokTypeLib_find_by_name(lib, "char");
    tmp = HavokTypeLib_find_by_name(lib, "const_char*");
    String_from_cstr(&tmp->name, "T*");
    tmp->is_ptr = true;
    tmp->is_primitive = false;
    HavokTemplateArgument *template_argument = DA_append_get(&tmp->template_arguments);
    String_from_cstr(&template_argument->name, "T");
    template_argument->is_class = true;
    template_argument->type_hash = charPtr->hash;

    fprintf(header_output, "// This file is autogenerated\n");
    fprintf(header_output, "#ifndef %s_GUARD\n", String_data(namespace));
    fprintf(header_output, "#define %s_GUARD\n\n", String_data(namespace));
    fprintf(header_output, "#include \"havok/havok_support_types.h\"\n\n");
    fprintf(header_output, "#include \"tag_file/havok_tag_file.h\"\n\n");

    fprintf(impl_output, "// This file is autogenerated\n");
    fprintf(impl_output, "#include \"%s\"\n\n", String_data(header_relative_path));
    DA_FORI(lib->types.values, i) {
        generate_type_def(lib, &lib->types.values.items[i], header_output);
    }
    printf("Got %d types\n", lib->types.values.count);
    fprintf(header_output, "#endif //%s_GUARD\n", String_data(namespace));

    DA_init(&lib->exported_hashes, uint64, lib->types.values.count);
    DA_FORI(lib->types.values, i) {
        generate_function_forward_defs(lib, &lib->types.values.items[i], header_output);
        generate_read_function_body(lib, &lib->types.values.items[i], impl_output);
        generate_print_function_body(lib, &lib->types.values.items[i], impl_output);
    }

    generate_function_table(lib, header_output, impl_output);
}
