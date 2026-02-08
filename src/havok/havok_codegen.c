// Created by RED on 12.10.2025.

#include "havok/havok_codegen.h"

#include <assert.h>

#include "platform/logger.h"
#include "utils/common.h"
#include "utils/hash_helper.h"

void generate_members(const Havok_TypeLibrary *lib, const HavokType *record_type, FILE *header_output,
                      uint32 *prev_member_offset, uint32 *prev_member_size) {
    if (record_type->parent_hash != 0) {
        const HavokType *parent_type = DM_get(&lib->types, record_type->parent_hash);
        generate_members(lib, parent_type, header_output, prev_member_offset, prev_member_size);
    }

    fprintf(header_output, "    // %s members\n", String_cstr(&record_type->name));
    DA_FORI(record_type->members, i) {
        const HavokRecordMember *member = &record_type->members.items[i];
        const HavokType *member_type = DM_get(&lib->types, member->type_hash);
        if (member_type == NULL) {
            GLog_Error("No member type data for member %s of type %s",
                       String_cstr(&member->name), String_cstr(&record_type->name));
            abort();
        }

        if (member->offset != 0) {
            // Compute inter padding
            const uint32 expected_offset = *prev_member_offset + *prev_member_size;
            if (member->offset > expected_offset) {
                const uint32 padding_size = member->offset - expected_offset;
                fprintf(header_output, "    uint8 _padding_%s[%d]; // inter-member padding\n",
                        String_cstr(&member->name), padding_size);
            }
        }

        if (member_type->type == HK_ARRAY) {
            fprintf(header_output, "    %s %s; // offset: %d, flags: %d, size: %d\n",
                    String_cstr(&member_type->name),
                    String_cstr(&member->name), member->offset, member->flags,
                    member_type->size);
        }
        else if (member_type->type == HK_FIXED_ARRAY) {
            const HavokType *inner_type = DM_get(&lib->types, member_type->inner_type_hash);
            fprintf(header_output, "    %s %s[%i]; // offset: %d, flags: %d, size: %d, hash: 0x%08X\n",
                    String_cstr(&inner_type->name),
                    String_cstr(&member->name), member_type->array_size,
                    member->offset, member->flags,
                    member_type->size, member_type->hash);
        }
        else {
            if (String_cequals(&member->name, "bool")) {
                fprintf(header_output, "    %s _bool; // offset: %d, flags: %d, size: %d\n",
                        String_cstr(&member_type->name), member->offset, member->flags,
                        member_type->size);
            }
            else {
                fprintf(header_output, "    %s %s; // offset: %d, flags: %d, size: %d\n",
                        String_cstr(&member_type->name),
                        String_cstr(&member->name), member->offset, member->flags,
                        member_type->size);
            }
        }

        *prev_member_offset = member->offset;
        *prev_member_size = member_type->size;
    }
}

void generate_ti_info(const HavokType *type, FILE *output,
                      const bool has_init,
                      const bool has_free,
                      const bool has_read,
                      const bool has_print,
                      const bool is_record,
                      const bool is_array
) {
    fprintf(output, "HavokTypeInfo %s_TI = {\n", String_cstr(&type->name));
    fprintf(output, "    .name = \"%s\",\n", String_cstr(&type->name));
    fprintf(output, "    .hash = 0x%08X,\n", type->hash);
    fprintf(output, "    .size = sizeof(%s),\n", String_cstr(&type->name));
    fprintf(output, "    .disk_size = %u,\n", type->size);
    fprintf(output, "    .is_record = %u,\n", is_record ? 1 : 0);
    fprintf(output, "    .is_array = %u,\n", is_array ? 1 : 0);
    if (has_init)
        fprintf(output, "    .init = (initHavokObject)%s_init,\n", String_cstr(&type->name));
    else
        fprintf(output, "    .init = NULL,\n");
    if (has_free)
        fprintf(output, "    .free = (freeHavokObject)%s_free,\n", String_cstr(&type->name));
    else
        fprintf(output, "    .free = NULL,\n");
    if (has_read)
        fprintf(output, "    .read = (readHavokObject)%s_read,\n", String_cstr(&type->name));
    else
        fprintf(output, "    .read = NULL,\n");
    if (has_print)
        fprintf(output, "    .print = (printHavokObject)%s_print,\n", String_cstr(&type->name));
    else
        fprintf(output, "    .print = NULL,\n");
    fprintf(output, "};\n\n");
}

bool is_complex_type(const Havok_TypeLibrary *lib, const HavokType *type) {
    if (type->type == HK_ARRAY || type->type == HK_PTR || type->type == HK_STRING) {
        return true;
    }
    if (type->type == HK_RECORD) {
        bool has_complex_members = false;
        if (type->parent_hash != 0) {
            const HavokType *parent = DM_get(&lib->types, type->parent_hash);
            has_complex_members |= is_complex_type(lib, parent);
        }
        DA_FORI(type->members, i) {
            const HavokRecordMember *member = &type->members.items[i];
            const HavokType *member_type = DM_get(&lib->types, member->type_hash);
            has_complex_members |= is_complex_type(lib, member_type);
        }
        return has_complex_members;
    }
    if (type->type == HK_PRIMITIVE && type->parent_hash != 0) {
        const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
        return is_complex_type(lib, parent_type);
    }
    return false;
}

void generate_type_def(const Havok_TypeLibrary *lib, const HavokType *type, FILE *header_output, FILE *impl_output) {
    if (DA_contains(&lib->exported_hashes, &type->hash, compare_hashes64)) {
        return;
    }
    DA_append(&lib->exported_hashes, &type->hash);

    if (type->parent_hash != 0) {
        const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
        generate_type_def(lib, parent_type, header_output, impl_output);
    }
    if (type->type == HK_RECORD) {
        if (type->members.count == 0 && type->parent_hash == 0) {
            return;
        }
        DA_FORI(type->members, i) {
            const HavokRecordMember *member = &type->members.items[i];
            const HavokType *member_type = DM_get(&lib->types, member->type_hash);
            if (member_type == NULL) {
                GLog_Error("No member type data for member %s of type %s",
                           String_cstr(&member->name), String_cstr(&type->name));
                abort();
            }
            generate_type_def(lib, member_type, header_output, impl_output);
        }

        bool has_complex_members = is_complex_type(lib, type);

        fprintf(header_output, "#define %s_HASH 0x%08X\n", String_cstr(&type->name), type->hash);
        fprintf(header_output, "typedef /*alignas(%i)*/ struct %s { // Record\n", type->align,
                String_cstr(&type->name));

        fprintf(header_output, "    HavokTypeInfo* type_info_;\n");

        uint32 prev_offset, prev_size;
        generate_members(lib, type, header_output, &prev_offset, &prev_size);
        // Final padding
        const uint32 expected_size = prev_offset + prev_size;
        if (expected_size < type->size) {
            const uint32 padding_size = type->size - expected_size;
            fprintf(header_output, "    uint8 _padding_end[%d]; // final padding\n", padding_size);
        }
        fprintf(header_output, "} %s;\n", String_cstr(&type->name));

        fprintf(header_output, "extern HavokTypeInfo %s_TI;\n\n", String_cstr(&type->name));

        generate_ti_info(type, impl_output,true, has_complex_members, true, true, true, false);

        // fprintf(header_output, "static_assert(sizeof(%s)==(%i+8)), \"Invalid size for %s\");\n\n",
        // String_data(&type->name), type->size, String_data(&type->name));
    }
    else if (type->type == HK_PTR) {
        const HavokType *inner_type = DM_get(&lib->types, type->inner_type_hash);
        generate_type_def(lib, inner_type, header_output, impl_output);
    }
    else if (type->type == HK_PRIMITIVE) {
        if (type->parent_hash != 0) {
            const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
            generate_type_def(lib, parent_type, header_output, impl_output);

            fprintf(header_output, "#define %s_HASH 0x%08X\n", String_cstr(&type->name), type->hash);
            fprintf(header_output, "typedef /*alignas(%i)*/ %s %s;\n\n",
                    type->align, String_cstr(&parent_type->name), String_cstr(&type->name));
            const bool complex_type = is_complex_type(lib, parent_type);
            generate_ti_info(type, impl_output, complex_type, complex_type, true, true, parent_type->type==HK_RECORD,parent_type->type==HK_ARRAY);
        }
        else {
            GLog_Error("Primitive without parent type is invalid!");
            abort();
        }
    }
    else if (type->type == HK_BASIC) {
        assert(type->parent_hash==0);
        assert(type->members.count==0);
        assert(type->inner_type_hash==0);
        fprintf(header_output, "// #define %s_HASH 0x%08X\n", String_cstr(&type->name), type->hash);
        fprintf(header_output, "// Basic type %s must be implemented by host\n", String_cstr(&type->name));
        fprintf(header_output, "// Type info: size: %u, alignment: %u\n\n", type->size, type->align);

        generate_ti_info(type, impl_output, false, false, true, true, false,false);
    }
    else if (type->type == HK_ENUM) {
        if (type->parent_hash != 0) {
            const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
            generate_type_def(lib, parent_type, header_output, impl_output);

            fprintf(header_output, "#define %s_HASH 0x%08X\n", String_cstr(&type->name), type->hash);
            fprintf(header_output, "typedef /*alignas(%i)*/ %s %s; // Enum\n\n",
                    type->align, String_cstr(&parent_type->name), String_cstr(&type->name));

            generate_ti_info(type, impl_output, false, false, true, true, false,false);
        }
        else {
            switch (type->size) {
                case 1: {
                    fprintf(header_output, "#define %s_HASH 0x%08X\n", String_cstr(&type->name), type->hash);
                    fprintf(header_output, "typedef /*alignas(%i)*/ uint8 %s; // Enum\n\n",
                            type->align, String_cstr(&type->name));
                    generate_ti_info(type, impl_output, false, false, true, true, false,false);

                    break;
                }
                case 2: {
                    fprintf(header_output, "#define %s_HASH 0x%08X\n", String_cstr(&type->name), type->hash);
                    fprintf(header_output, "typedef /*alignas(%i)*/ uint16 %s; // Enum\n\n",
                            type->align, String_cstr(&type->name));
                    generate_ti_info(type, impl_output, false, false, true, true, false,false);
                    break;
                }
                case 4: {
                    fprintf(header_output, "#define %s_HASH 0x%08X\n", String_cstr(&type->name), type->hash);
                    fprintf(header_output, "typedef /*alignas(%i)*/ uint32 %s; // Enum\n\n",
                            type->align, String_cstr(&type->name));
                    generate_ti_info(type, impl_output, false, false, true, true, false,false);
                    break;
                }
                default: {
                    GLog_Error("Unsupported enum size %d for type %s", type->size, String_cstr(&type->name));
                    abort();
                }
            }
        }
    }
    else if (type->type == HK_FIXED_ARRAY) {
        const HavokType *inner_type = DM_get(&lib->types, type->inner_type_hash);
        generate_type_def(lib, inner_type, header_output, impl_output);

        fprintf(impl_output, "HavokTypeInfo %s_TI = {\n", String_cstr(&type->name));
        fprintf(impl_output, "    .name = \"%s\",\n", String_cstr(&type->name));
        fprintf(impl_output, "    .hash = 0x%08X,\n", type->hash);
        fprintf(impl_output, "    .size = sizeof(%s)*%u,\n", String_cstr(&inner_type->name), type->array_size);
        fprintf(impl_output, "    .disk_size = %u*%u,\n", inner_type->size, type->array_size);
        fprintf(impl_output, "    .is_record = 0,\n");
        fprintf(impl_output, "    .is_array = 0,\n");
        fprintf(impl_output, "    .init = NULL,\n");
        fprintf(impl_output, "    .free = NULL,\n");
        fprintf(impl_output, "    .read = (readHavokObject)%s_read,\n", String_cstr(&type->name));
        fprintf(impl_output, "    .print = (printHavokObject)%s_print,\n", String_cstr(&type->name));
        fprintf(impl_output, "};\n\n");
    }
    else if (type->type == HK_ARRAY) {
        const HavokType *inner_type = DM_get(&lib->types, type->inner_type_hash);
        generate_type_def(lib, inner_type, header_output, impl_output);
        fprintf(header_output, "#define %s_HASH 0x%08X\n", String_cstr(&type->name), type->hash);
        fprintf(header_output, "typedef /*alignas(%i)*/ struct %s { // Array\n",
                type->align, String_cstr(&type->name));
        fprintf(header_output, "    HavokTypeInfo* inner_type_info;\n");
        fprintf(header_output, "    %s* m_data;\n", String_cstr(&inner_type->name));
        fprintf(header_output, "    uint32 m_size;\n");
        fprintf(header_output, "    uint32 m_capacityAndFlags;\n");
        fprintf(header_output, "} %s;\n", String_cstr(&type->name));

        generate_ti_info(type, impl_output, true, true, true, true, false, true);
    }
    else if (type->type == HK_STRING) {
        fprintf(header_output, "#define %s_HASH 0x%08X\n", String_cstr(&type->name), type->hash);
        // fprintf(header_output, "typedef /*alignas(%i)*/ struct %s { // String\n",
        //         type->align, String_data(&type->name));
        // fprintf(header_output, "    char* m_data;\n");
        // fprintf(header_output, "} %s;\n\n", String_data(&type->name));

        generate_ti_info(type, impl_output, false, true, true, true, false,false);
    }
    else {
        GLog_Error("Unhandled type kind %i for type %s", type->type, String_cstr(&type->name));
        abort();
    }
}

void generate_function_forward_defs(Havok_TypeLibrary *lib, const HavokType *type, FILE *header_output) {
    const String *full_name = &type->name;
    if (type->type == HK_RECORD) {
        if (type->members.count == 0 && type->parent_hash == 0) {
            return;
        }

        bool is_complex = is_complex_type(lib, type);

        fprintf(header_output,
                "void %s_init(%s *obj);\n",
                String_cstr(full_name),
                String_cstr(full_name)
        );
        fprintf(header_output,
                "void %s_read(%s *obj, const TagFile *tf, const uint8* src);\n",
                String_cstr(full_name),
                String_cstr(full_name));
        fprintf(header_output,
                "void %s_print(const %s *obj, JsonContext *ctx);\n",
                String_cstr(full_name), String_cstr(full_name));
        if (is_complex) {
            fprintf(header_output,
                    "void %s_free(%s *obj);\n\n",
                    String_cstr(full_name),
                    String_cstr(full_name));
        }
    }
    else if (type->type == HK_PRIMITIVE) {
        if (type->parent_hash == 0) {
            GLog_Error("Primitive type %s has no parent type!", String_cstr(full_name));
            return;
        }
        const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
        const bool complex_type = is_complex_type(lib, parent_type);
        if (complex_type) {
            fprintf(header_output,
                    "void %s_init(%s *obj);\n",
                    String_cstr(full_name),
                    String_cstr(full_name)
            );
        }
        fprintf(header_output,
                "void %s_read(%s *obj, const TagFile *tf, const uint8* src);\n",
                String_cstr(full_name),
                String_cstr(full_name));
        fprintf(header_output,
                "void %s_print(const %s *obj, JsonContext *ctx);\n",
                String_cstr(full_name), String_cstr(full_name));
        if (complex_type) {
            fprintf(header_output,
                    "void %s_free(%s *obj);\n\n",
                    String_cstr(full_name),
                    String_cstr(full_name));
        }
    }
    else if (type->type == HK_ENUM) {
        fprintf(header_output,
                "void %s_read( %s *obj, const TagFile *tf, const uint8* src);\n",
                String_cstr(full_name),
                String_cstr(full_name));
        fprintf(header_output, "void %s_print(const %s *obj, JsonContext *ctx);\n\n",
                String_cstr(full_name), String_cstr(full_name));
    }
    else if (type->type == HK_ARRAY) {
        fprintf(header_output,
                "void %s_init(%s *obj);\n",
                String_cstr(full_name),
                String_cstr(full_name)
        );
        fprintf(header_output,
                "void %s_read(%s *obj, const TagFile *tf, const uint8* src);\n",
                String_cstr(full_name),
                String_cstr(full_name));
        fprintf(header_output, "void %s_print(const %s *obj, JsonContext *ctx);\n",
                String_cstr(full_name), String_cstr(full_name));
        fprintf(header_output,
                "void %s_free(%s *obj);\n\n",
                String_cstr(full_name),
                String_cstr(full_name));
    }
    else if (type->type == HK_FIXED_ARRAY) {
        const HavokType *inner_type = DM_get(&lib->types, type->inner_type_hash);
        fprintf(header_output,
                "void %s_read(%s *obj, const TagFile *tf, const uint8* src);\n",
                String_cstr(full_name),
                String_cstr(&inner_type->name));
        fprintf(header_output, "void %s_print(const %s *obj, JsonContext *ctx);\n\n",
                String_cstr(full_name),
                String_cstr(&inner_type->name));
    }
}

void generate_read_function_body(Havok_TypeLibrary *lib, const HavokType *type, FILE *impl_out) {
    const String *type_name = &type->name;
    if (type->type == HK_RECORD) {
        if (type->members.count == 0 && type->parent_hash == 0) {
            return;
        }
        fprintf(impl_out,
                "void %s_read(%s *obj, const TagFile *tf,const uint8* src) {\n",
                String_cstr(type_name),
                String_cstr(type_name));
        if (type->parent_hash != 0) {
            const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
            if (parent_type->members.count != 0 || parent_type->parent_hash != 0) {
                fprintf(impl_out, "    %s_read((%s*)obj, tf, src);\n",
                        String_cstr(&parent_type->name),
                        String_cstr(&parent_type->name));
            }
        }
        DA_FORI(type->members, i) {
            const HavokRecordMember *member = &type->members.items[i];
            const HavokType *member_type = DM_get(&lib->types, member->type_hash);
            if (member_type == NULL) {
                GLog_Error("No member type data for member %s of type %s",
                           String_cstr(&member->name), String_cstr(&type->name));
                abort();
            }
            if (member_type->type == HK_PTR) {
                fprintf(impl_out, "    ptr_read((void**)&obj->%s, tf, src + %i, NULL);\n",
                        String_cstr(&member->name), member->offset);
            }
            else if (member_type->type == HK_FIXED_ARRAY) {
                fprintf(impl_out, "    %s_read(obj->%s, tf, src + %i);\n",
                        String_cstr(&member_type->name),
                        String_cstr(&member->name),
                        member->offset);
            }
            else if (member_type->type == HK_RECORD) {
                if (member_type->members.count == 0 && member_type->parent_hash == 0) {
                    continue;
                }
                fprintf(impl_out, "    %s_read(&obj->%s, tf, src + %i);\n",
                        String_cstr(&member_type->name),
                        String_cstr(&member->name), member->offset);
            }
            else if (member_type->type == HK_ARRAY) {
                fprintf(impl_out, "    hkArray_read(&obj->%s, tf, src + %i);\n",
                        String_cstr(&member->name), member->offset);
            }
            else {
                fprintf(impl_out, "    %s_read(&obj->%s, tf, src + %i);\n",
                        String_cstr(&member_type->name),
                        String_cstr(&member->name), member->offset);
            }
        }
        fprintf(impl_out, "}\n\n");
    }
    // else if (type->type == HK_STRING) {
    //     fprintf(impl_out,
    //             "void %s_read(%s *obj, const TagFile *tf, const uint8* src) {\n",
    //             String_data(type_name),
    //             String_data(type_name));
    //     fprintf(impl_out, "    ptr_read((void**)&obj->m_data, tf, src, NULL);\n");
    //     fprintf(impl_out, "}\n\n");
    // }
    else if (type->type == HK_PRIMITIVE) {
        if (type->parent_hash == 0) {
            GLog_Error("Primitive type %s has no parent type!", String_cstr(type_name));
            return;
        }
        const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
        fprintf(impl_out,
                "void %s_read(%s *obj, const TagFile *tf, const uint8* src) {\n",
                String_cstr(type_name),
                String_cstr(type_name));
        fprintf(impl_out, "    %s_read((%s*)obj, tf, src);\n",
                String_cstr(&parent_type->name),
                String_cstr(&parent_type->name));
        fprintf(impl_out, "}\n\n");
    }
    else if (type->type == HK_ENUM) {
        if (type->parent_hash != 0) {
            const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
            fprintf(impl_out,
                    "void %s_read(%s *obj, const TagFile *tf, const uint8* src) {\n",
                    String_cstr(type_name),
                    String_cstr(type_name));
            fprintf(impl_out, "    %s_read((%s*)obj, tf , src);\n",
                    String_cstr(&parent_type->name),
                    String_cstr(&parent_type->name));
            fprintf(impl_out, "}\n\n");
        }
        else {
            switch (type->size) {
                case 1: {
                    fprintf(impl_out,
                            "void %s_read(%s *obj, const TagFile *tf, const uint8* src) {\n",
                            String_cstr(type_name),
                            String_cstr(type_name));
                    fprintf(impl_out, "    *obj = *((uint8*)src);\n");
                    fprintf(impl_out, "}\n\n");
                    break;
                }
                case 2: {
                    fprintf(impl_out,
                            "void %s_read(%s *obj, const TagFile *tf, const uint8* src) {\n",
                            String_cstr(type_name),
                            String_cstr(type_name));
                    fprintf(impl_out, "    *obj = *((uint16*)src);\n");
                    fprintf(impl_out, "}\n\n");
                    break;
                }
                case 4: {
                    fprintf(impl_out,
                            "void %s_read(%s *obj, const TagFile *tf, const uint8* src) {\n",
                            String_cstr(type_name),
                            String_cstr(type_name));
                    fprintf(impl_out, "    *obj = *((uint32*)src);\n");
                    fprintf(impl_out, "}\n\n");
                    break;
                }
                default: {
                    GLog_Error("Unsupported enum size %d for type %s", type->size, String_cstr(type_name));
                    abort();
                }
            }
        }
    }
    else if (type->type == HK_FIXED_ARRAY) {
        const HavokType *inner_type = DM_get(&lib->types, type->inner_type_hash);
        fprintf(impl_out,
                "void %s_read(%s *obj, const TagFile *tf, const uint8* src) {\n",
                String_cstr(type_name),
                String_cstr(&inner_type->name));
        fprintf(impl_out, "    for (uint32 i = 0; i < %i; ++i) {\n", type->array_size);
        fprintf(impl_out, "        %s_read(&obj[i], tf, src + i * %i);\n",
                String_cstr(&inner_type->name), inner_type->size);
        fprintf(impl_out, "    }\n");
        fprintf(impl_out, "}\n\n");
    }
    else if (type->type == HK_ARRAY) {
        fprintf(impl_out,
                "void %s_read(%s *obj, const TagFile *tf, const uint8* src) {\n",
                String_cstr(type_name),
                String_cstr(type_name));
        fprintf(impl_out, "    hkArray_read(obj, tf, src);\n");
        fprintf(impl_out, "}\n\n");
    }
}

bool ancestral_is_record(Havok_TypeLibrary *lib, const HavokType *type) {
    if (type->type == HK_RECORD) {
        return true;
    }
    if (type->parent_hash != 0) {
        const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
        return ancestral_is_record(lib, parent_type);
    }
    return false;
}

bool ancestral_is_array(Havok_TypeLibrary *lib, const HavokType *type) {
    if (type->type == HK_ARRAY) {
        return true;
    }
    if (type->parent_hash != 0) {
        const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
        return ancestral_is_array(lib, parent_type);
    }
    return false;
}

void generate_print_function_body(Havok_TypeLibrary *lib, const HavokType *type, FILE *impl_out) {
    const String *type_name = &type->name;
    if (type->type == HK_RECORD) {
        if (type->members.count == 0 && type->parent_hash == 0) {
            return;
        }
        fprintf(impl_out,
                "void %s_print(const %s *obj, JsonContext *ctx) {\n",
                String_cstr(type_name),
                String_cstr(type_name));
        if (type->parent_hash != 0) {
            const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
            if (parent_type->members.count != 0 || parent_type->parent_hash != 0) {
                fprintf(impl_out, "    %s_print((%s*)obj, ctx);\n",
                        String_cstr(&parent_type->name),
                        String_cstr(&parent_type->name));
            }
        }
        DA_FORI(type->members, i) {
            const HavokRecordMember *member = &type->members.items[i];
            const HavokType *member_type = DM_get(&lib->types, member->type_hash);
            if (member_type == NULL) {
                GLog_Error("No member type data for member %s of type %s",
                           String_cstr(&member->name), String_cstr(&type->name));
                abort();
            }
            fprintf(impl_out, "    jsonName(ctx, \"%s\");\n", String_cstr(&member->name));
            if (member_type->type == HK_PTR) {
                const HavokType *inner_type = DM_get(&lib->types, member_type->inner_type_hash);
                fprintf(impl_out, "    if(obj->%s != NULL) {\n", String_cstr(&member->name));
                if (ancestral_is_record(lib, inner_type)) {
                    fprintf(impl_out, "        jsonBeginObject(ctx);\n");
                    fprintf(impl_out, "        obj->%s->type_info_->print(obj->%s, ctx);\n", String_cstr(&member->name),
                            String_cstr(&member->name));
                    fprintf(impl_out, "        jsonEndObject(ctx);\n");
                }
                else {
                    fprintf(impl_out, "        obj->%s->type_info_->print(obj->%s, ctx);\n", String_cstr(&member->name),
                            String_cstr(&member->name));
                }
                fprintf(impl_out, "    } else {\n");
                fprintf(impl_out, "        jsonValueNull(ctx);\n");
                fprintf(impl_out, "    }\n");
            }
            else if (member_type->type == HK_FIXED_ARRAY) {
                fprintf(impl_out, "    %s_print(obj->%s, ctx);\n",
                        String_cstr(&member_type->name),
                        String_cstr(&member->name)
                );
            }
            else if (ancestral_is_record(lib, member_type)) {
                if (member_type->members.count == 0 && member_type->parent_hash == 0) {
                    continue;
                }
                if (String_cequals(&member_type->name,"hkBool")) {
                    fprintf(impl_out, "    jsonValueBool(ctx,obj->%s._bool>0);\n",
                            String_cstr(&member->name));
                }else {
                    fprintf(impl_out, "    jsonBeginObject(ctx);\n");
                    fprintf(impl_out, "    %s_print(&obj->%s, ctx);\n",
                            String_cstr(&member_type->name),
                            String_cstr(&member->name));
                    fprintf(impl_out, "    jsonEndObject(ctx);\n");
                }
            }
            else if (ancestral_is_array(lib, member_type)) {
                fprintf(impl_out, "    hkArray_print(&obj->%s, ctx);\n", String_cstr(&member->name));
            }
            else {
                fprintf(impl_out, "    %s_print(&obj->%s, ctx);\n",
                        String_cstr(&member_type->name),
                        String_cstr(&member->name));
            }
        }
        fprintf(impl_out, "}\n\n");
    }
    else if (type->type == HK_PRIMITIVE) {
        if (type->parent_hash == 0) {
            GLog_Error("Primitive type %s has no parent type!", String_cstr(type_name));
            return;
        }
        const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
        fprintf(impl_out,
                "void %s_print(const %s *obj, JsonContext *ctx) {\n",
                String_cstr(type_name),
                String_cstr(type_name));
        fprintf(impl_out, "    %s_print((%s*)obj, ctx);\n",
                String_cstr(&parent_type->name),
                String_cstr(&parent_type->name));
        fprintf(impl_out, "}\n\n");
    }
    else if (type->type == HK_ENUM) {
        if (type->parent_hash != 0) {
            const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
            fprintf(impl_out,
                    "void %s_print(const %s *obj, JsonContext *ctx) {\n",
                    String_cstr(type_name),
                    String_cstr(type_name));
            fprintf(impl_out, "    %s_print((%s*)obj, ctx);\n",
                    String_cstr(&parent_type->name),
                    String_cstr(&parent_type->name));
            fprintf(impl_out, "}\n\n");
        }
        else {
            switch (type->size) {
                case 1:
                case 2:
                case 4: {
                    fprintf(impl_out,
                            "void %s_print(const %s *obj, JsonContext *ctx) {\n",
                            String_cstr(type_name),
                            String_cstr(type_name));
                    fprintf(impl_out, "    jsonValueUInt(ctx, *obj);\n");
                    fprintf(impl_out, "}\n\n");
                    break;
                }
                default: {
                    GLog_Error("Unsupported enum size %d for type %s", type->size, String_cstr(type_name));
                    abort();
                }
            }
        }
    }
    else if (type->type == HK_STRING) {
        // fprintf(impl_out,
        //         "void %s_print(const %s *obj, JsonContext *ctx) {\n",
        //         String_cstr(type_name),
        //         String_cstr(type_name));
        // fprintf(impl_out, "    if (obj->m_data != NULL) {\n");
        // fprintf(impl_out, "        jsonValueStr(ctx, obj->m_data);\n");
        // fprintf(impl_out, "    } else {\n");
        // fprintf(impl_out, "        jsonValueNull(ctx);\n");
        // fprintf(impl_out, "    }\n");
        // fprintf(impl_out, "}\n\n");
    }
    else if (type->type == HK_ARRAY) {
        fprintf(impl_out,
                "void %s_print(const %s *obj, JsonContext *ctx) {\n",
                String_cstr(type_name),
                String_cstr(type_name));
        fprintf(impl_out, "    hkArray_print(obj, ctx);\n");
        fprintf(impl_out, "}\n\n");
    }
    else if (type->type == HK_PTR) {
        // Do nothing, it should be handled by record
    }
    else if (type->type == HK_BASIC) {
        // Do nothing, basic types are implemented by host
    }
    else if (type->type == HK_FIXED_ARRAY) {
        const HavokType *inner_type = DM_get(&lib->types, type->inner_type_hash);
        fprintf(impl_out,
                "void %s_print(const %s *obj, JsonContext *ctx) {\n",
                String_cstr(type_name),
                String_cstr(&inner_type->name));
        fprintf(impl_out, "    jsonBeginArray(ctx);\n");
        fprintf(impl_out, "    for (uint32 i = 0; i < %i; ++i) {\n", type->array_size);
        fprintf(impl_out, "        %s_print(&obj[i], ctx);\n",
                String_cstr(&inner_type->name));
        fprintf(impl_out, "    }\n");
        fprintf(impl_out, "    jsonEndArray(ctx);\n");
        fprintf(impl_out, "}\n\n");
    }
    else {
        GLog_Error("Unhandled type kind %i for type %s", type->type, String_cstr(&type->name));
        abort();
    }
}

void generate_free_function_body(Havok_TypeLibrary *lib, const HavokType *type, FILE *impl_file) {
    const String *type_name = &type->name;
    if (type->type == HK_RECORD) {
        if (type->members.count == 0 && type->parent_hash == 0 || !is_complex_type(lib, type)) {
            return;
        }
        fprintf(impl_file,
                "void %s_free(%s *obj) {\n",
                String_cstr(type_name),
                String_cstr(type_name));

        DA_FORI(type->members, i) {
            const HavokRecordMember *member = &type->members.items[i];
            const HavokType *member_type = DM_get(&lib->types, member->type_hash);
            if (member_type == NULL) {
                GLog_Error("No member type data for member %s of type %s",
                           String_cstr(&member->name), String_cstr(&type->name));
                abort();
            }
            const bool is_complex = is_complex_type(lib, member_type);
            if (!is_complex) {
                continue;
            }

            if (member_type->type == HK_PTR) {
                fprintf(impl_file, "    ptr_free(obj->%s);\n",
                        String_cstr(&member->name)
                );
            }
            else if (member_type->type == HK_STRING) {
                fprintf(impl_file, "    if(obj->%s.m_data!=NULL) mp_free(obj->%s.m_data);\n",
                        String_cstr(&member->name),
                        String_cstr(&member->name));
            }
            else if (member_type->type == HK_ARRAY) {
                fprintf(impl_file, "    hkArray_free(&obj->%s);\n",
                        String_cstr(&member->name)
                );
            }
            else if (member_type->type == HK_RECORD) {
                if (member_type->members.count == 0 && member_type->parent_hash == 0) {
                    continue;
                }
                fprintf(impl_file, "    %s_free(&obj->%s);\n",
                        String_cstr(&member_type->name),
                        String_cstr(&member->name));
            }
        }
        if (type->parent_hash != 0) {
            const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
            if ((parent_type->members.count != 0 || parent_type->parent_hash != 0) &&
                is_complex_type(lib, parent_type)) {
                fprintf(impl_file, "    %s_free((%s*)obj);\n",
                        String_cstr(&parent_type->name),
                        String_cstr(&parent_type->name));
            }
        }
        fprintf(impl_file, "}\n\n");
    }
    else if (type->type == HK_PRIMITIVE) {
        if (type->parent_hash == 0) {
            GLog_Warning("Primitive type %s has no parent type!", String_cstr(type_name));
            return;
        }
        const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
        const bool complex_type = is_complex_type(lib, parent_type);
        if (complex_type) {
            fprintf(impl_file,
                    "void %s_free(%s *obj) {\n",
                    String_cstr(type_name),
                    String_cstr(type_name));
            fprintf(impl_file, "    %s_free((%s*)obj);\n",
                    String_cstr(&parent_type->name),
                    String_cstr(&parent_type->name));
            fprintf(impl_file, "}\n\n");
        }
    }
    // else if (type->type == HK_STRING) {
    //     fprintf(impl_file,
    //             "void %s_free(%s *obj) {\n",
    //             String_data(type_name),
    //             String_data(type_name));
    //     fprintf(impl_file, "    if (obj->m_data != NULL) {\n");
    //     fprintf(impl_file, "        mp_free((void*)obj->m_data);\n");
    //     fprintf(impl_file, "        obj->m_data = NULL;\n");
    //     fprintf(impl_file, "    }\n");
    //     fprintf(impl_file, "}\n\n");
    // }
    else if (type->type == HK_ARRAY) {
        fprintf(impl_file,
                "void %s_free(%s *obj) {\n",
                String_cstr(type_name),
                String_cstr(type_name));
        fprintf(impl_file, "    hkArray_free(obj);\n");
        fprintf(impl_file, "}\n\n");
    }
}

void generate_init_function_body(Havok_TypeLibrary *lib, const HavokType *type, FILE *impl_output) {
    const String *type_name = &type->name;
    const HavokType *inner_type = DM_get(&lib->types, type->inner_type_hash);
    if (type->type == HK_RECORD) {
        if (type->members.count == 0 && type->parent_hash == 0) {
            return;
        }
        fprintf(impl_output,
                "void %s_init(%s *obj) {\n",
                String_cstr(type_name),
                String_cstr(type_name));
        fprintf(impl_output, "    memset(obj, 0, sizeof(%s));\n",
                String_cstr(type_name));
        if (type->parent_hash != 0) {
            const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
            if (parent_type->members.count != 0 || parent_type->parent_hash != 0) {
                fprintf(impl_output, "    %s_init((%s*)obj);\n",
                        String_cstr(&parent_type->name),
                        String_cstr(&parent_type->name));
            }
        }
        DA_FORI(type->members, i) {
            const HavokRecordMember *member = &type->members.items[i];
            const HavokType *member_type = DM_get(&lib->types, member->type_hash);
            if (member_type == NULL) {
                GLog_Error("No member type data for member %s of type %s",
                           String_cstr(&member->name), String_cstr(&type->name));
                abort();
            }
            if (member_type->type == HK_RECORD) {
                if (member_type->members.count == 0 && member_type->parent_hash == 0) {
                    continue;
                }
                fprintf(impl_output, "    %s_init(&obj->%s);\n",
                        String_cstr(&member_type->name),
                        String_cstr(&member->name));
            }
            else if (member_type->type == HK_PRIMITIVE) {
                if (member_type->parent_hash == 0) {
                    continue;
                }
                const HavokType *parent_type = DM_get(&lib->types, member_type->parent_hash);
                const bool complex_type = is_complex_type(lib, parent_type);
                if (complex_type) {
                    fprintf(impl_output, "    %s_init(&obj->%s);\n",
                            String_cstr(&member_type->name),
                            String_cstr(&member->name));
                }
            }
            else if (member_type->type == HK_ARRAY) {
                fprintf(impl_output, "    %s_init(&obj->%s);\n",
                        String_cstr(&member_type->name),
                        String_cstr(&member->name));
            }
        }
        fprintf(impl_output, "    obj->type_info_ = &%s_TI;\n", String_cstr(type_name));
        fprintf(impl_output, "}\n\n");
    }
    else if (type->type == HK_ARRAY) {
        fprintf(impl_output,
                "void %s_init(%s *obj) {\n",
                String_cstr(type_name),
                String_cstr(type_name));
        fprintf(impl_output, "    memset(obj, 0, sizeof(%s));\n",
                String_cstr(type_name));
        fprintf(impl_output, "    obj->inner_type_info = &%s_TI;\n", String_cstr(&inner_type->name));
        fprintf(impl_output, "}\n\n");
    }
    else if (type->type == HK_PRIMITIVE) {
        if (type->parent_hash == 0) {
            GLog_Error("Primitive type %s has no parent type!", String_cstr(type_name));
            return;
        }
        const HavokType *parent_type = DM_get(&lib->types, type->parent_hash);
        const bool complex_type = is_complex_type(lib, parent_type);
        if (complex_type) {
            fprintf(impl_output,
                    "void %s_init(%s *obj) {\n",
                    String_cstr(type_name),
                    String_cstr(type_name));
            fprintf(impl_output, "    %s_init((%s*)obj);\n",
                    String_cstr(&parent_type->name),
                    String_cstr(&parent_type->name));
            fprintf(impl_output, "}\n\n");
        }
    }
}

void generate_function_table(const Havok_TypeLibrary *lib, FILE *header_output, FILE *impl_output,
                             const String *namespace) {

    fprintf(header_output, "void %s_register_functions();\n", String_cstr(namespace));

    fprintf(impl_output, "TypeInfoMap %s_type_info = {0};\n\n", String_cstr(namespace));

    fprintf(impl_output, "void register_type_info(TypeInfoMap *map, uint32 hash, const HavokTypeInfo *type_info){\n");
    fprintf(impl_output, "    const HavokTypeInfo **slot = DM_insert(map, hash);\n");
     fprintf(impl_output, "    *slot = type_info;\n");
    fprintf(impl_output, "}\n");


    fprintf(impl_output, "void %s_register_functions() {\n", String_cstr(namespace));
    fprintf(impl_output, "    DM_init(&%s_type_info, HavokTypeInfo*, %u);\n", String_cstr(namespace),
            lib->types.keys.count);
    DA_FORI(lib->types.values, i) {
        const HavokType *type = &lib->types.values.items[i];
        if (type->type == HK_RECORD && type->members.count == 0 && type->parent_hash == 0) {
            continue;
        }
        if (type->type == HK_PTR) {
            continue;
        }
        const String *full_name = &type->name;
        fprintf(impl_output,
                "    register_type_info(&%s_type_info, 0x%08X, &%s_TI);\n",
                String_cstr(namespace),
                type->hash,
                String_cstr(full_name));
    }
    fprintf(impl_output, "}\n\n");
}

void Havok_TypeLibrary_generate_code(Havok_TypeLibrary *lib, const String *namespace, FILE *header_output,
                                     const String *header_relative_path, FILE *impl_output) {
    const HavokType *ull = Havok_TypeLibrary_find_by_name(lib, "unsigned_long_long");
    HavokType *khbase_object = Havok_TypeLibrary_find_by_name(lib, "hkBaseObject");
    khbase_object->hash = hash_string(&khbase_object->name);
    khbase_object->type = HK_RECORD;
    HavokRecordMember *fake_member = DA_append_get(&khbase_object->members);
    String_from_cstr(&fake_member->name, "unk_member");
    fake_member->offset = 0;
    fake_member->flags = 0;
    fake_member->type_hash = ull->hash;

    fprintf(header_output, "// This file is autogenerated\n");
    fprintf(header_output, "#ifndef %s_GUARD\n", String_cstr(namespace));
    fprintf(header_output, "#define %s_GUARD\n\n", String_cstr(namespace));
    fprintf(header_output, "#define ALLOC_DEBUG\n");
    fprintf(header_output, "#ifndef TRACY_MEMORY\n#define TRACY_MEMORY\n#endif\n");
    fprintf(header_output, "#include \"havok/havok_helpers.h\"\n\n");
    fprintf(header_output, "#include \"havok/havok_support_types.h\"\n\n");

    fprintf(impl_output, "// This file is autogenerated\n");
    fprintf(impl_output, "#include \"%s\"\n\n", String_cstr(header_relative_path));

    DA_init(&lib->exported_hashes, uint64, lib->types.values.count);

    DA_FORI(lib->types.values, i) {
        const HavokType *type = &lib->types.values.items[i];
        if (type->type == HK_RECORD) {
            fprintf(header_output, "typedef struct %s %s;\n", String_cstr(&type->name), String_cstr(&type->name));
        }
    }

    DA_FORI(lib->types.values, i) {
        generate_function_forward_defs(lib, &lib->types.values.items[i], impl_output);
    }
    DA_FORI(lib->types.values, i) {
        generate_type_def(lib, &lib->types.values.items[i], header_output, impl_output);
    }
    printf("Got %d types\n", lib->types.values.count);

    DA_init(&lib->exported_hashes, uint64, lib->types.values.count);
    DA_FORI(lib->types.values, i) {
        const HavokType *type = &lib->types.values.items[i];
        generate_init_function_body(lib, type, impl_output);
        generate_read_function_body(lib, type, impl_output);
        generate_print_function_body(lib, type, impl_output);
        generate_free_function_body(lib, type, impl_output);
    }

    fprintf(header_output, "extern TypeInfoMap %s_type_info;\n\n", String_cstr(namespace));
    generate_function_table(lib, header_output, impl_output, namespace);
    fprintf(header_output, "#endif //%s_GUARD\n", String_cstr(namespace));

    DA_free(&lib->exported_hashes);
}
