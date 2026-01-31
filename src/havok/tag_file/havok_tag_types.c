// Created by RED on 12.10.2025.


#include "havok/tag_file/havok_tag_types.h"

#include <assert.h>

#include "platform/logger.h"
#include "utils/hash_helper.h"

void HKTagTypeMember_init(HKTagTypeMember *member, const String *name) {
    const char* raw_buffer = String_cstr(name);
    if (raw_buffer[0] >= '0' && raw_buffer[0] <= '9') {
        String_from_cstr(&member->name, "_");
        String_append_str(&member->name, name);
    } else
        String_copy_from(&member->name, name);
}

void HKTagTypeMember_free(HKTagTypeMember *member) {
    String_free(&member->name);
}

void HKTagTemplateArgument_init(HKTagTemplateArgument *arg, const String *name) {
    const char* raw_buffer = String_cstr(name);
    if (raw_buffer[0] == 't') {
        String_sub_string(name, 1, -1, &arg->name);
        arg->is_class = 1;
    } else if (raw_buffer[0] == 'v') {
        String_sub_string(name, 1, -1, &arg->name);
        arg->is_number = 1;
    } else
        String_copy_from(&arg->name, name);
}

void HKTagTemplateArgument_free(HKTagTemplateArgument *arg) {
    String_free(&arg->name);
}

void process_template_arg(const HKTagTemplateArgument *arg, String *out) {
    if (arg->is_class) {
        assert(arg->type != NULL);
        if (String_cequals(&arg->type->name, "hkContainerHeapAllocator")) {
            return;
        }
        String_append_cstr(out, "_");
        const HKTagType* inner_type = arg->type;
        if (inner_type->template_args.count==0) {
            String_append_str(out, &arg->type->name);
        }else {
            String_append_str(out, &inner_type->name);
            DA_FORI(inner_type->template_args, k) {
                const HKTagTemplateArgument *inner_arg = &inner_type->template_args.items[k];
                process_template_arg(inner_arg, out);
            }
        }

    } else if (arg->is_number) {
        String_append_format(out, "_%i", arg->number);
    } else {
        String_append_cstr(out, "_");
        String_append_str(out, &arg->name);
    }
}

void create_hk_array_name(const HKTagType *hk_type, String *out) {
    String_copy_from(out, &hk_type->name);
    DA_FORI(hk_type->template_args, j) {
        const HKTagTemplateArgument *arg = &hk_type->template_args.items[j];
        process_template_arg(arg, out);
    }
}

void create_hk_ptr_name(const HKTagType *hk_type, String *out) {
    if (hk_type->template_args.count > 1) {
        GLog_Error("Pointer type with more than one template argument!");
        String_from_cstr(out, "INVALID_PTR_TYPE");
        return;
    }
    const HKTagTemplateArgument *arg = &hk_type->template_args.items[0];
    const String *inner_type_name = HKTagType_stable_name(arg->type);
    String_copy_from(out, inner_type_name);
    String_append_cstr(out, "*");
}

void create_hk_fixed_array_name(const HKTagType *hk_type, String *out) {
    String_copy_from(out, &hk_type->template_args.items[0].type->name);
    for (uint32 j = 1; j < (hk_type->template_args).count; j++) {
        const HKTagTemplateArgument *arg = &hk_type->template_args.items[j];
        process_template_arg(arg, out);
    }
}

void create_hk_enum_name(const HKTagType *hk_type, String *out) {
    String_copy_from(out, &hk_type->name);
    DA_FORI(hk_type->template_args, j) {
        const HKTagTemplateArgument *arg = &hk_type->template_args.items[j];
        process_template_arg(arg, out);
    }
}

void create_templated_record_name(const HKTagType *hk_type, String *out) {
    String_copy_from(out, &hk_type->name);
    DA_FORI(hk_type->template_args, j) {
        const HKTagTemplateArgument *arg = &hk_type->template_args.items[j];
        process_template_arg(arg, out);
    }
}

const String *HKTagType_stable_name(HKTagType *tf_type) {
    if (String_size(&tf_type->stable_name)>0) {
        return &tf_type->stable_name;
    }
    String *name = &tf_type->stable_name;

    if ((tf_type->members.count > 0 || tf_type->data_type == HKTYPE_RECORD) && tf_type->template_args.count == 0) {
        String_copy_from(name, &tf_type->name);
    } else if ((tf_type->members.count > 0 || tf_type->data_type == HKTYPE_RECORD) && tf_type->template_args.count >
               0) {
        create_templated_record_name(tf_type, name);
    } else if (String_cstarts_with(&tf_type->name, "hkArray")) {
        create_hk_array_name(tf_type, name);
    } else if (String_cstarts_with(&tf_type->name, "void") && tf_type->data_type == HKTYPE_PRIMITIVE) {
        String_copy_from(name, &tf_type->name);
    } else if (String_cequals(&tf_type->name, "T*")) {
        if (String_cequals(&tf_type->template_args.items[0].type->name, "void")) {
            String_from_cstr(name, "voidPtr");
        } else {
            create_hk_ptr_name(tf_type, name);
        }
    } else if (String_cstarts_with(&tf_type->name, "T[N]")) {
        create_hk_fixed_array_name(tf_type, name);
    } else if (String_cequals(&tf_type->name, "hkStringPtr")) {
        String_copy_from(name, &tf_type->name);
    } else if (String_cequals(&tf_type->name, "const_char*")) {
        String_from_cstr(name, "const_charPtr");
    } else if (String_cequals(&tf_type->name, "hkEnum") ||
               String_cequals(&tf_type->name, "hkFlags")) {
        create_hk_enum_name(tf_type, name);
    } else if (tf_type->data_type == HKTYPE_PRIMITIVE && tf_type->template_args.count) {
        String_copy_from(name, &tf_type->name);
        for (int i = 0; i < tf_type->template_args.count; ++i) {
        process_template_arg(&tf_type->template_args.items[i], name);

        }
    } else if (tf_type->data_type == HKTYPE_PRIMITIVE) {
        String_copy_from(name, &tf_type->name);
    } else if (tf_type->data_type == HKTYPE_BASIC || tf_type->data_type == HKTYPE_OPAQUE ||
               tf_type->data_type == HKTYPE_FLOAT) {
        String_copy_from(name, &tf_type->name);
    } else if (String_cequals(&tf_type->name, "hkVector4f") || String_cequals(&tf_type->name, "hkRotationImpl")) {
        String_copy_from(name, &tf_type->name);
    } else {
        GLog_Warning("Unhandled case for stable name -> %s  [%s]", String_cstr(&tf_type->name),
                     HKTAGTYPE_NAMES[tf_type->data_type]);
        String_copy_from(name, &tf_type->name);
    }
    return name;
}

uint32 HKTagType_hash(HKTagType *type) {
    const String *stable_name = HKTagType_stable_name(type);
    const uint32 hash = hash_string(stable_name);
    return hash;
}

void HKTagType_init(HKTagType *type, const String *name) {
    String_copy_from(&type->name, name);
    String_replace_char(&type->name, ": ", '_');
    String_free(&type->stable_name);

    DA_init(&type->template_args, HKTagTemplateArgument, 1);
    DA_init(&type->members, HKTagTypeMember, 1);
    type->parent = NULL;
    DA_init(&type->interfaces, HKTagInterface, 1);
}

void HKTagType_free(HKTagType *type) {
    String_free(&type->name);
    String_free(&type->stable_name);
    DA_free_with_inner(&type->members, {HKTagTypeMember_free(it);});
    DA_free(&type->interfaces);
    DA_free_with_inner(&type->template_args, { HKTagTemplateArgument_free(it);});
}
