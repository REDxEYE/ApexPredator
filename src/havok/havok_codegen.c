// Created by RED on 12.10.2025.

#include "havok/havok_codegen.h"
#include "../../include/havok/tag_file/havok_tag_types.h"
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
    if (type->name.buffer[0] == 'T' && type->name.buffer[1] == '*') {
        assert(type->template_arguments.count==1);
        const HavokType *inner_type = DM_get(&lib->types, type->template_arguments.items[0].type_hash);
        String_append_str(full_name, &inner_type->name);
        String_append_cstr(full_name, "*");
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
}

void HavokTypeLib_free(HavokTypeLib *lib) {
    for (int i = 0; i < lib->types.values.count; ++i) {
        HavokType *type = DA_at(&lib->types.values, i);
        HavokType_free(type);
    }
    DM_free(&lib->types);
    DA_free(&lib->exported_hashes);
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
        // Enums first template arg is it's actual type. It need to be processed and treated as enum;
        const HKTagTemplateArgument *enum_type_arg = &tf_type->template_args.items[0];
        HavokType* inner_enum_type = HavokTypeLib__register_type(lib, enum_type_arg->type);
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
        new_type->is_array = 1;
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

void generate_type_def(const HavokTypeLib *lib, const HavokType *type, FILE *header_output, bool forward_declare) {
    if (DA_contains(&lib->exported_hashes, &type->hash, compare_hashes64)) {
        return;
    }
    DA_append(&lib->exported_hashes, &type->hash);

    if (String_cequals(&type->name, "T*")) {
        return;
    }

    String *full_name = Havok_full_type_name(lib, type);
    if (type->is_record) {
        if (forward_declare) {
            fprintf(header_output, "typedef struct %s %s;\n\n", String_data(full_name), String_data(full_name));
            return;
        }
        DA_FORI(type->members, i) {
            const HavokRecordMember *member = &type->members.items[i];
            const HavokType *member_type = DM_get(&lib->types, member->type_hash);
            if (member_type != NULL) {
                generate_type_def(lib, member_type, header_output, forward_declare);
            }
        }
        if (type->parent_hash != 0) {
            const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
            if (parent_type != NULL) {
                generate_type_def(lib, parent_type, header_output, forward_declare);
            }
        }
        fprintf(header_output, "#define %s_HASH 0x%08X\n", String_data(full_name), type->hash);
        fprintf(header_output, "typedef struct %s {\n", String_data(full_name));
        if (type->parent_hash != 0) {
            const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
            String *parent_full_name = Havok_full_type_name(lib, parent_type);
            fprintf(header_output, "    struct %s;\n", String_data(parent_full_name));
            String_free(parent_full_name);
        }
        DA_FORI(type->members, i) {
            const HavokRecordMember *member = &type->members.items[i];
            const HavokType *member_type = DM_get(&lib->types, member->type_hash);
            if (member_type == NULL) {
                fprintf(header_output, "    /* %s: UNKNOWN_TYPE */; // offset: %d, flags: %d\n",
                        String_data(&member->name), member->offset, member->flags);
            } else {
                String *member_full_name = Havok_full_type_name(lib, member_type);
                if (member_type->is_array) {
                    if (member_type->template_arguments.count == 2) {
                        const HavokTemplateArgument *inner_type_arg = &member_type->template_arguments.items[0];
                        const HavokTemplateArgument *size_arg = &member_type->template_arguments.items[1];
                        assert(inner_type_arg->is_class);
                        const HavokType *inner_type = DM_get(&lib->types, inner_type_arg->type_hash);
                        assert(inner_type!=NULL);
                        String *inner_full_name = Havok_full_type_name(lib, inner_type);
                        if (size_arg->is_number) {
                            fprintf(header_output, "    %s %s[%u]; // offset: %d, flags: %d, size: %d\n",
                                    String_data(inner_full_name),
                                    String_data(&member->name), size_arg->number, member->offset, member->flags,
                                    member_type->size);
                        } else {
                            fprintf(header_output, "    %s %s; // offset: %d, flags: %d, size: %d\n",
                                    String_data(member_full_name),
                                    String_data(&member->name), member->offset, member->flags,
                                    member_type->size);
                        }
                        String_free(inner_full_name);
                    } else if (member_type->template_arguments.count == 0) {
                        // Normal type that is internally an array
                        fprintf(header_output, "    %s %s; // offset: %d, flags: %d, size: %d\n",
                                String_data(member_full_name),
                                String_data(&member->name), member->offset, member->flags,
                                member_type->size);
                    } else {
                        assert(false);
                    }
                } else {
                    fprintf(header_output, "    %s %s; // offset: %d, flags: %d, size: %d\n",
                            String_data(member_full_name),
                            String_data(&member->name), member->offset, member->flags,
                            member_type->size);
                }
                String_free(member_full_name);
            }
        }
        fprintf(header_output, "} %s;\n\n", String_data(full_name));
        String_free(full_name);
        return;
    } else if (type->is_enum) {
        if (forward_declare) {
            fprintf(header_output, "typedef enum %s %s;\n\n", String_data(full_name), String_data(full_name));
            return;
        }
        fprintf(header_output, "#define %s_HASH 0x%08X\n", String_data(full_name), type->hash);
        fprintf(header_output, "typedef enum %s { // size: %d\n", String_data(full_name), type->size);
        DA_FORI(type->members, i) {
            const HavokRecordMember *member = &type->members.items[i];
            fprintf(header_output, "    %s = %u,\n", String_data(&member->name), member->offset);
        }
        fprintf(header_output, "} %s;\n\n", String_data(full_name));
        String_free(full_name);
        return;
    } else if (type->is_primitive) {
        if (type->parent_hash == 0) {
            fprintf(header_output, "/* primitive %s*/\n", String_data(full_name));
            return;
        }
        if (forward_declare) {
            const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
            generate_type_def(lib, parent_type, header_output, forward_declare);
            String *parent_full_name = Havok_full_type_name(lib, parent_type);
            fprintf(header_output, "typedef %s %s;\n\n", String_data(parent_full_name), String_data(full_name));
            String_free(parent_full_name);
            String_free(full_name);
            return;
        }
        const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
        generate_type_def(lib, parent_type, header_output, forward_declare);
        String *parent_full_name = Havok_full_type_name(lib, parent_type);
        fprintf(header_output, "#define %s_HASH 0x%08X\n", String_data(full_name), type->hash);
        fprintf(header_output, "typedef %s %s;\n\n", String_data(parent_full_name), String_data(full_name));
        String_free(parent_full_name);
        String_free(full_name);
        return;
    }

    if (type->parent_hash != 0 && forward_declare) {
        const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
        generate_type_def(lib, parent_type, header_output, forward_declare);
        String *parent_full_name = Havok_full_type_name(lib, parent_type);
        fprintf(header_output, "typedef %s %s;\n\n", String_data(parent_full_name), String_data(full_name));
        String_free(parent_full_name);
        String_free(full_name);
        return;
    }
    if (String_cequals(full_name, "void")) {
        String_free(full_name);
        return;
    }
    if (String_cequals(full_name, "short")) {
        String_free(full_name);
        return;
    }
    if (String_cequals(full_name, "float")) {
        String_free(full_name);
        return;
    }
    if (String_cequals(full_name, "signed_char")) {
        String_free(full_name);
        return;
    }
    printf("[WARN]: Skipping non-struct type %s\n", String_data(full_name));
    String_free(full_name);
    // exit(1);
}

void HavokTypeLib_generate_code(HavokTypeLib *lib, const String *namespace, FILE *header_output,
                                const String *header_relative_path, FILE *impl_output) {
    DA_init(&lib->exported_hashes, uint64, lib->types.values.count);

    // Register aliases for common types:
    // All spaces in havok types are replaced with _, so we need to register extra remap types to generate valid code
    // unsigned_int -> unsigned int
    register_alias(lib, "unsigned_int", "unsigned int");
    register_alias(lib, "unsigned_short", "unsigned short");
    register_alias(lib, "unsigned_char", "unsigned char");
    register_alias(lib, "signed_int", "int");
    register_alias(lib, "signed_short", "short");
    register_alias(lib, "signed_char", "char");
    register_alias(lib, "unsigned_long_long", "unsigned long long");
    register_alias(lib, "const_char*", "const char*");
    register_alias(lib, "hkVector4f", "HavokVector4");

    fprintf(header_output, "// This file is autogenerated\n");
    fprintf(header_output, "#ifndef %s_GUARD\n", String_data(namespace));
    fprintf(header_output, "#define %s_GUARD\n\n", String_data(namespace));
    fprintf(header_output, "#include \"havok/havok_support_types.h\"\n\n");

    DA_FORI(lib->types.values, i) {
        generate_type_def(lib, &lib->types.values.items[i], header_output, true);
    }
    DA_init(&lib->exported_hashes, uint64, lib->types.values.count);
    DA_FORI(lib->types.values, i) {
        generate_type_def(lib, &lib->types.values.items[i], header_output, false);
    }
    printf("Got %d types\n", lib->types.values.count);
    fprintf(header_output, "#endif //%s_GUARD\n", String_data(namespace));
}
