// Created by RED on 12.10.2025.


#include "../../../include/havok/tag_file/havok_tag_types.h"

void HKTagTypeMember_init(HKTagTypeMember *member, const String *name) {
    if (name->buffer[0] >= '0' && name->buffer[0] <= '9') {
        String_from_cstr(&member->name, "_");
        String_append_str(&member->name, name);
    } else
        String_copy_from(&member->name, name);
}

void HKTagTypeMember_free(HKTagTypeMember *member) {
    String_free(&member->name);
}

void HKTagTemplateArgument_init(HKTagTemplateArgument *arg, const String *name) {
    if (name->buffer[0] == 't') {
        String_sub_string(name, 1, -1, &arg->name);
        arg->is_class = 1;
    } else if (name->buffer[0] == 'v') {
        String_sub_string(name, 1, -1, &arg->name);
        arg->is_number = 1;
    } else
        String_copy_from(&arg->name, name);
}

void HKTagTemplateArgument_free(HKTagTemplateArgument *arg) {
    String_free(&arg->name);
}

void HKTagType_init(HKTagType *type, const String *name) {
    String_copy_from(&type->name, name);
    for (int i = 0; i < type->name.size; ++i) {
        if (type->name.buffer[i] == ':' || type->name.buffer[i] == ' ') {
            type->name.buffer[i] = '_';
        }
    }
    DA_init(&type->template_args, HKTagTemplateArgument, 1);
    DA_init(&type->members, HKTagTypeMember, 1);
    type->parent = NULL;
    DA_init(&type->interfaces, HKTagInterface, 1);
}

void HKTagType_free(HKTagType *type) {
    String_free(&type->name);
    DA_free_with_inner(&type->members, {HKTagTypeMember_free(it);});
    DA_free(&type->interfaces);
    DA_free_with_inner(&type->template_args, { HKTagTemplateArgument_free(it);});
}
