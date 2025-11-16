// Created by RED on 10.10.2025.

#include "../../../include/havok/tag_file/havok_tag_file.h"

#include <math.h>

#include "platform/common_arrays.h"
#include "utils/buffer/memory_buffer.h"
#include "utils/endian.h"

TagHeader expect_tag(Buffer *buffer, const char *expected_ident) {
    TagHeader header;
    if (!TagHeader_from_buffer(&header, buffer)) {
        printf("[ERROR]: Failed to read tag header\n");
        exit(1);
    }
    if (memcmp(header.ident, expected_ident, 4) != 0) {
        printf("[ERROR]: Expected tag %.4s but got %.4s\n", expected_ident, header.ident);
        exit(1);
    }
    return header;
}

int64 read_compressed_int(Buffer *buffer) {
    uint8 b0, b1, b2, b3, b4, b5, b6, b7, b8;

    if (buffer->read(buffer, &b0, 1, NULL) != BUFFER_SUCCESS) {
        return 0;
    }
    if ((b0 & 0x80) == 0) {
        return b0;
    }

    uint8 b0_shift3 = b0 >> 3;
    switch (b0_shift3) {
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:
            if (buffer->read(buffer, &b1, 1, NULL) != BUFFER_SUCCESS) { return 0; }

            return ((b0 << 8) | b1) & 0x3FFF;
        case 0x18:
        case 0x19:
        case 0x1A:
        case 0x1B:
            if (buffer->read(buffer, &b1, 1, NULL) != BUFFER_SUCCESS ||
                buffer->read(buffer, &b2, 1, NULL) != BUFFER_SUCCESS) { return 0; }

            return ((b0 << 16) | (b1 << 8) | b2) & 0x1FFFFF;
        case 0x1C:
            if (buffer->read(buffer, &b1, 1, NULL) != BUFFER_SUCCESS ||
                buffer->read(buffer, &b2, 1, NULL) != BUFFER_SUCCESS ||
                buffer->read(buffer, &b3, 1, NULL) != BUFFER_SUCCESS) { return 0; }

            return ((b0 << 24) | (b1 << 16) | (b2 << 8) | b3) & 0x7FFFFFF;
        case 0x1D:
            if (buffer->read(buffer, &b1, 1, NULL) != BUFFER_SUCCESS ||
                buffer->read(buffer, &b2, 1, NULL) != BUFFER_SUCCESS ||
                buffer->read(buffer, &b3, 1, NULL) != BUFFER_SUCCESS ||
                buffer->read(buffer, &b4, 1, NULL) != BUFFER_SUCCESS) { return 0; }

            return (((uint64) b0 << 32) | ((uint64) b1 << 24) | ((uint64) b2 << 16) | ((uint64) b3 << 8) | b4) &
                   0x7FFFFFFFFULL;
        case 0x1E:
            if (buffer->read(buffer, &b1, 1, NULL) != BUFFER_SUCCESS ||
                buffer->read(buffer, &b2, 1, NULL) != BUFFER_SUCCESS ||
                buffer->read(buffer, &b3, 1, NULL) != BUFFER_SUCCESS ||
                buffer->read(buffer, &b4, 1, NULL) != BUFFER_SUCCESS ||
                buffer->read(buffer, &b5, 1, NULL) != BUFFER_SUCCESS ||
                buffer->read(buffer, &b6, 1, NULL) != BUFFER_SUCCESS ||
                buffer->read(buffer, &b7, 1, NULL) != BUFFER_SUCCESS) { return 0; }

            return (((uint64) b0 << 56) | ((uint64) b1 << 48) | ((uint64) b2 << 40) | ((uint64) b3 << 32) |
                    ((uint64) b4 << 24) | ((uint64) b5 << 16) | ((uint64) b6 << 8) | b7) & 0x7FFFFFFFFFFFFFFULL;
        case 0x1F: {
            int v6 = b0 & 7;
            if (v6 == 0) {
                if (buffer->read(buffer, &b1, 1, NULL) != BUFFER_SUCCESS ||
                    buffer->read(buffer, &b2, 1, NULL) != BUFFER_SUCCESS ||
                    buffer->read(buffer, &b3, 1, NULL) != BUFFER_SUCCESS ||
                    buffer->read(buffer, &b4, 1, NULL) != BUFFER_SUCCESS ||
                    buffer->read(buffer, &b5, 1, NULL) != BUFFER_SUCCESS) { return 0; }

                return (((uint64) b0 << 40) | ((uint64) b1 << 32) | ((uint64) b2 << 24) | ((uint64) b3 << 16) |
                        ((uint64) b4 << 8) | b5) & 0xFFFFFFFFFFULL;
            }
            if (v6 == 1) {
                if (buffer->read(buffer, &b1, 1, NULL) != BUFFER_SUCCESS ||
                    buffer->read(buffer, &b2, 1, NULL) != BUFFER_SUCCESS ||
                    buffer->read(buffer, &b3, 1, NULL) != BUFFER_SUCCESS ||
                    buffer->read(buffer, &b4, 1, NULL) != BUFFER_SUCCESS ||
                    buffer->read(buffer, &b5, 1, NULL) != BUFFER_SUCCESS ||
                    buffer->read(buffer, &b6, 1, NULL) != BUFFER_SUCCESS ||
                    buffer->read(buffer, &b7, 1, NULL) != BUFFER_SUCCESS ||
                    buffer->read(buffer, &b8, 1, NULL) != BUFFER_SUCCESS) { return 0; }

                return (((uint64) b1 << 56) | ((uint64) b2 << 48) | ((uint64) b3 << 40) | ((uint64) b4 << 32) |
                        ((uint64) b5 << 24) | ((uint64) b6 << 16) | ((uint64) b7 << 8) | b8);
            }

            return 0;
        }
        default:{}
    }

    if ((b0 & 0xc0) == 0x80) {
        if (buffer->read(buffer, &b1, 1, NULL) != BUFFER_SUCCESS) { return 0; }

        return ((b0 & 0x3f) << 8) | b1;
    }
    if ((b0 & 0xe0) == 0xc0) {
        if (buffer->read(buffer, &b1, 1, NULL) != BUFFER_SUCCESS ||
            buffer->read(buffer, &b2, 1, NULL) != BUFFER_SUCCESS) { return 0; }

        return ((b0 & 0x1f) << 16) | (b1 << 8) | b2;
    }
    if ((b0 & 0xf0) == 0xe0) {
        if (buffer->read(buffer, &b1, 1, NULL) != BUFFER_SUCCESS ||
            buffer->read(buffer, &b2, 1, NULL) != BUFFER_SUCCESS ||
            buffer->read(buffer, &b3, 1, NULL) != BUFFER_SUCCESS) { return 0; }

        return ((b0 & 0x0f) << 24) | (b1 << 16) | (b2 << 8) | b3;
    }

    return 0;
}

int32 read_compressed_int3(Buffer *buffer) {
    uint8 firstInt;
    int32 resultInt = 0;

    if (buffer->read(buffer, &firstInt, 1, NULL) != BUFFER_SUCCESS) {
        printf("[ERROR]: Failed to read first byte\n");
        return -1;
    }

    bool flag1 = (firstInt & 0x80) == 0x80;
    bool flag2 = (firstInt & 0xC0) == 0xC0;
    bool flag3 = (firstInt & 0xE0) == 0xE0;

    if (flag3) {
        if (buffer->read(buffer, &resultInt, 4, NULL) != BUFFER_SUCCESS) {
            printf("[ERROR]: Failed to read compressed int\n");
            return -1;
        }
        resultInt |= (firstInt & 0xf) << 4;
    } else if (flag2) {
        printf("[ERROR]: Unhandled int compression : 0xC0!\n");
        return -1;
    } else if (flag1) {
        uint8 secondInt;
        if (buffer->read(buffer, &secondInt, 1, NULL) != BUFFER_SUCCESS) {
            printf("[ERROR]: Failed to read second byte\n");
            return -1;
        }
        resultInt = secondInt | (((int32) firstInt & 0xf) << 8);
    } else {
        resultInt = firstInt;
    }

    return resultInt;
}

uint32 read_compressed_int2(Buffer *buffer) {
    uint8 b0;
    if (buffer->read(buffer, &b0, 1, NULL) != BUFFER_SUCCESS) {
        printf("[ERROR]: Failed to read first byte\n");
        return -1;
    }
    if ((b0 & 0x80) == 0) {
        return b0;
    }

    uint8 b1, b2, b3;
    uint32 result = 0;
    uint8 b0_shift3 = b0 >> 3;

    if (b0_shift3 >= 0x10 && b0_shift3 <= 0x17) {
        if (buffer->read(buffer, &b1, 1, NULL) != BUFFER_SUCCESS) {
            printf("[ERROR]: Failed to read second byte\n");
            return -1;
        }
        result = ((b0 << 8) | b1) & 0x3fff;
        return result;
    }
    if (b0_shift3 >= 0x18 && b0_shift3 <= 0x1B) {
        if (buffer->read(buffer, &b1, 1, NULL) != BUFFER_SUCCESS ||
            buffer->read(buffer, &b2, 1, NULL) != BUFFER_SUCCESS) {
            printf("[ERROR]: Failed to read bytes\n");
            return -1;
        }
        result = ((b0 << 16) | (b1 << 8) | b2) & 0x1fffff;
        return result;
    }

    if ((b0 & 0xc0) == 0x80) {
        if (buffer->read(buffer, &b1, 1, NULL) != BUFFER_SUCCESS) {
            printf("[ERROR]: Failed to read second byte\n");
            return -1;
        }
        result = ((b0 & 0x3f) << 8) | b1;
        return result;
    }
    if ((b0 & 0xe0) == 0xc0) {
        if (buffer->read(buffer, &b1, 1, NULL) != BUFFER_SUCCESS ||
            buffer->read(buffer, &b2, 1, NULL) != BUFFER_SUCCESS) {
            printf("[ERROR]: Failed to read bytes\n");
            return -1;
        }
        result = ((b0 & 0x1f) << 16) | (b1 << 8) | b2;
        return result;
    }
    if ((b0 & 0xf0) == 0xe0) {
        if (buffer->read(buffer, &b1, 1, NULL) != BUFFER_SUCCESS ||
            buffer->read(buffer, &b2, 1, NULL) != BUFFER_SUCCESS ||
            buffer->read(buffer, &b3, 1, NULL) != BUFFER_SUCCESS) {
            printf("[ERROR]: Failed to read bytes\n");
            return -1;
        }
        result = ((b0 & 0x0f) << 24) | (b1 << 16) | (b2 << 8) | b3;
        return result;
    }
    printf("[ERROR]: Unsupported packed value: 0x%02x\n", b0);
    return -1;
}


void print_indent(FILE *out, uint32 indent) {
    for (uint32 i = 0; i < indent; ++i) {
        fputc(' ', out);
    }
}

void print_type_variable(const HKTagType *type, FILE *out) {
    fprintf(out, "%s", String_data(&type->name));
    DA_FORI(type->template_args, j) {
        const HKTagTemplateArgument *arg = &type->template_args.items[j];
        if (arg->is_class) {
            assert(arg->type != NULL);
            fprintf(out, "_%s", String_data(&arg->type->name));
        } else if (arg->is_number) {
            fprintf(out, "_%u", arg->number);
        } else {
            fprintf(out, "_%s", String_data(&arg->name));
        }
    }
}

void HKTagType_print(const HKTagType *type, FILE *out, uint32 indent) {
    if (type->members.count > 0) {
        // fprintf(out, "struct %s { // size: %d, flags: %d\n", String_data(&type->name), type->size, type->flags);
        fprintf(out, "typedef struct ");
        print_type_variable(type, out);
        fprintf(out, " { // size: %d, flags: %d, type: %s, format: %i\n", type->size, type->flags,
                HKTAGTYPE_NAMES[type->data_type], type->format);
        if (type->parent != NULL) {
            print_indent(out, indent + 4);
            fprintf(out, "struct %s;\n", String_data(&type->parent->name));
        }
        DA_FORI(type->members, i) {
            const HKTagTypeMember *member = &type->members.items[i];
            print_indent(out, indent + 4);
            if (member->type == NULL) {
                fprintf(out, "/* %s: UNKNOWN_TYPE */;\n", String_data(&member->name));
            } else {
                if (member->type->template_args.count > 0) {
                    if (member->type->data_type == HKTYPE_ARRAY) {
                        assert(member->type->template_args.count==2);
                        const HKTagTemplateArgument *inner_type = &member->type->template_args.items[0];
                        const HKTagTemplateArgument *size_arg = &member->type->template_args.items[1];
                        assert(inner_type->is_class && inner_type->type!=NULL);
                        if (size_arg->is_number) {
                            fprintf(out, "%s %s[%u]; // offset: %d, flags: %d, size: %d\n",
                                    String_data(&inner_type->type->name),
                                    String_data(&member->name), size_arg->number, member->offset, member->flags,
                                    member->type->size);
                        } else {
                            print_type_variable(member->type, out);

                            fprintf(out, " %s; // offset: %d, flags: %d, size: %d\n",
                                    String_data(&member->name), member->offset,
                                    member->flags, member->type->size);
                        }
                    } else {
                        if (member->type->data_type == HKTYPE_POINTER) {
                            fprintf(out, "ItemId %s_id; // offset: %d, flags: %d, size: %d\n",
                                    // String_data(&member->type->template_args.items[0].type->name),
                                    String_data(&member->name), member->offset, member->flags, member->type->size);
                        } else {
                            fprintf(out, "%s", String_data(&member->type->name));
                            for (uint32 j = 0; j < member->type->template_args.count; ++j) {
                                const HKTagTemplateArgument *arg = &member->type->template_args.items[j];
                                if (arg->is_class) {
                                    assert(arg->type != NULL);
                                    fprintf(out, "_%s", String_data(&arg->type->name));
                                } else if (arg->is_number) {
                                    fprintf(out, "_%u", arg->number);
                                } else {
                                    fprintf(out, "_%s", String_data(&arg->name));
                                }
                            }
                            fprintf(out, " %s; // offset: %d, flags: %d, size: %d\n", String_data(&member->name),
                                    member->offset, member->flags, member->type->size);

                            // fprintf(out, "/* %s: GENERIC_TYPE */; // offset: %d, flags: %d, size: %d\n",
                            //         String_data(&member->name), member->offset, member->flags, member->type->size);
                        }
                    }
                } else {
                    fprintf(out, "%s %s; // offset: %d, flags: %d, size: %d\n", String_data(&member->type->name),
                            String_data(&member->name), member->offset, member->flags, member->type->size);
                }
            }
        }
        fprintf(out, "} ");
        print_type_variable(type, out);
        fprintf(out, ";\n");
        return;
    }

    switch (type->data_type) {
        case HKTYPE_POINTER: {
            assert(type->template_args.count==1);
            const HKTagTemplateArgument *inner_type = &type->template_args.items[0];
            assert(inner_type->is_class && inner_type->type!=NULL);
            // fprintf(out, "/* POINTER %s* */", String_data(&inner_type->type->name));
            break;
        }
        case HKTYPE_RECORD: {
            if (type->parent != NULL) {
                fprintf(out, "typedef struct %s %s; // Size: %d", String_data(&type->parent->name),
                        String_data(&type->name), type->size);
            } else {
                fprintf(out, "typedef struct %s {\n", String_data(&type->name));
                print_indent(out, indent + 4);
                switch (type->size) {
                    case 1: {
                        fprintf(out, "uint8 unk0;\n");
                        break;
                    }
                    case 8: {
                        fprintf(out, "uint64 unk0;\n");
                        break;
                    }
                    default: {
                        fprintf(out, "uint8 data[%d];\n", type->size);
                        break;
                    }
                }

                fprintf(out, "} %s; // Size: %d", String_data(&type->name), type->size);
                // fprintf(out, "/* EMPTY Record %s */", String_data(&type->name));
            }
            break;
        }
        case HKTYPE_STRING: {
            // fprintf(out, "/* Simple string %s */", String_data(&type->name));
            break;
        }
        case HKTYPE_UNK5:
        case HKTYPE_PRIMITIVE: {
            if (type->parent != NULL) {
                fprintf(out, "typedef %s ", String_data(&type->parent->name));
                print_type_variable(type, out);
                fprintf(out, ";");
            } else {
                // fprintf(out, "/* PRIMITIVE %s */", String_data(&type->name));
            }
            break;
        }
        case HKTYPE_UNK4: {
            // fprintf(out, "/* UNK4 %s */", String_data(&type->name));
            break;
        }
        case HKTYPE_UNK1: {
            // fprintf(out, "/* UNK1 %s */", String_data(&type->name));
            break;
        }
        case HKTYPE_ARRAY: {
            if (type->template_args.count == 2) {
                assert(type->template_args.count==2);
                const HKTagTemplateArgument *inner_type = &type->template_args.items[0];
                const HKTagTemplateArgument *size_arg = &type->template_args.items[1];
                assert(inner_type->is_class && inner_type->type!=NULL);
                if (size_arg->is_number) {
                    // fprintf(out, "/* ARRAY %s[%u] */", String_data(&inner_type->type->name), size_arg->number);
                } else {
                    // fprintf(out, "/* ARRAY %s[%s] */", String_data(&inner_type->type->name),
                    //         String_data(&size_arg->name));
                }
            } else if (type->template_args.count == 0) {
                // fprintf(out, "/* ARRAY %s */", String_data(&type->name));
            }
            break;
        }
        default: {
            printf("[ERROR]: Unsupported data type: %s\n", HKTAGTYPE_NAMES[type->data_type]);
            exit(1);
        };
    }
    fprintf(out, "\n");
}


bool TagHeader_from_buffer(TagHeader *header, Buffer *buffer) {
    uint32 size_and_flags;
    if (buffer->read_uint32(buffer, &size_and_flags) != BUFFER_SUCCESS)return false;
    // Size is havok tag files is in big endian;
    size_and_flags = BE32TOH(size_and_flags);
    header->size = (size_and_flags & 0x00FFFFFF);
    header->flags = size_and_flags >> 24;
    if (buffer->read(buffer, header->ident, 4, NULL) != BUFFER_SUCCESS)return false;
    return true;
}

bool SkipChunk_from_buffer(Buffer *buffer) {
    TagHeader header;
    TagHeader_from_buffer(&header, buffer);
    if (buffer->skip(buffer, header.size - 8) != BUFFER_SUCCESS)return false;
    return true;
}

bool SDKVTag_from_buffer(TagFile *tf, Buffer *buffer) {
    const TagHeader type_header = expect_tag(buffer, "SDKV");
    if (type_header.size - 8 < 8) {
        printf("[ERROR]: Invalid SDKV tag size: %d\n", type_header.size);
        return false;
    }
    if (buffer->read(buffer, tf->ver, 8, NULL) != BUFFER_SUCCESS)return false;
    return true;
}

bool DATATag_from_buffer(TagFile *tf, Buffer *buffer) {
    const TagHeader data_header = expect_tag(buffer, "DATA");
    DA_init(&tf->data, uint8, data_header.size-8);
    if (buffer->read(buffer, tf->data.items, data_header.size - 8, NULL) != BUFFER_SUCCESS)return false;
    return true;
}

bool Strings_from_buffer(const TagHeader *header, DynamicArray_String *strings, Buffer *buffer) {
    DA_init(strings, String, 1);
    uint32 strings_data_left = header->size - 8;
    while (strings_data_left > 0) {
        String *str = DA_append_get(strings);
        if (buffer->read_cstring(buffer, str) != BUFFER_SUCCESS)return false;
        strings_data_left -= (str->size + 1);
    }
    Buffer_align(buffer, 4);
    return true;
}

#define CHECK_RANGE(index, da, message)  \
    if (index < 0 || index >= (int64) da->count) { \
        printf("[ERROR]: "message": %lli\n", index); \
        return false; \
    }

bool read_type_identity(MemoryBuffer *mb, DynamicArray_HKTagType *types, const DynamicArray_String *class_names) {
    Buffer *buffer = (Buffer *) mb;
    const uint32 type_count = read_compressed_int(buffer);
    if (type_count == 0) {
        printf("[ERROR]: Type count is zero\n");
        exit(1);
    }
    DA_init(types, HKTagType, type_count);
    types->count = type_count;
    for (int i = 1; i < type_count; ++i) {
        HKTagType *type = DA_at(types, i);
        const int64 name_id = read_compressed_int(buffer);
        const int64 template_args_count = read_compressed_int(buffer);
        CHECK_RANGE(name_id, class_names, "Invalid type name id");
        HKTagType_init(type, &class_names->items[name_id]);
        DA_reserve(&type->template_args, template_args_count);
        for (int j = 0; j < template_args_count; ++j) {
            const int64 template_name_id = read_compressed_int(buffer);
            const int64 template_type_id = read_compressed_int(buffer);
            CHECK_RANGE(template_name_id, class_names, "Invalid template arg name id");
            HKTagTemplateArgument *arg = DA_append_get(&type->template_args);
            HKTagTemplateArgument_init(arg, &class_names->items[template_name_id]);
            if (arg->is_class) {
                if (template_type_id == 0) {
                    printf("[ERROR]: Template type id cannot be zero\n");
                    return false;
                }

                if (template_type_id < 0 || template_type_id >= (int32) types->count) {
                    printf("[ERROR]: ""Invalid template arg type id"": %lli\n", template_type_id);
                    return false;
                }
                HKTagType *template_type = &types->items[template_type_id];
                arg->type = template_type;
            } else if (arg->is_number) {
                arg->number = template_type_id;
            } else {
                printf("[ERROR]: Unsupported template arg type\n");
                return false;
            }
        }
    }
    Buffer_align(buffer, 4);
    BufferError error;
    const uint64 buffer_remaining = Buffer_remaining(buffer, &error);
    if (buffer_remaining > 0) {
        uint64 buffer_size;
        buffer->getsize(buffer, &buffer_size);
        printf("[ERROR]: TNAM did not read entire buffer, expected %lld but got %lld\n", buffer_size,
               buffer_size - buffer_remaining);
        return false;
    }
    return true;
}

bool read_type_body(MemoryBuffer *mb, const DynamicArray_HKTagType *types, const DynamicArray_String *member_names) {
    Buffer *buffer = (Buffer *) mb;
    BufferError error;
    while (Buffer_remaining(buffer, &error) > 0) {
        if (error != BUFFER_SUCCESS) {
            printf("[ERROR]: Failed to get remaining size of TBOD data\n");
            return false;
        }
        const int64 type_id = read_compressed_int(buffer);
        CHECK_RANGE(type_id, types, "Invalid type id in TBOD");
        if (type_id == 0)
            break;
        const int64 parent_id = read_compressed_int(buffer);
        CHECK_RANGE(type_id, types, "Invalid type id in TBOD");
        HKTagType *type = &types->items[type_id];
        if (parent_id != 0) {
            type->parent = &types->items[parent_id];
        }
        const HKTagFlags flags = read_compressed_int(buffer);
        if (flags & Format) {
            int64 format = read_compressed_int(buffer);
            uint8 data_type = format & 0xF;
            type->format = format >> 4;
            type->data_type = data_type;
        }
        if (flags & SubType) {
            type->sub_type = read_compressed_int(buffer);
        }
        if (flags & Version) {
            type->version = read_compressed_int(buffer);
        }
        if (flags & SizeAlign) {
            type->size = read_compressed_int(buffer);
            type->align = read_compressed_int(buffer);
        }

        if (flags & Flags) {
            type->flags = read_compressed_int(buffer);
        }

        if (flags & Fields) {
            const uint32 field_count = read_compressed_int(buffer);
            DA_reserve(&type->members, field_count);
            for (int i = 0; i < field_count; ++i) {
                const int64 member_name_id = read_compressed_int(buffer);
                const int64 member_flags = read_compressed_int(buffer);
                const int64 member_offset = read_compressed_int(buffer);
                const int64 member_unk = read_compressed_int(buffer);
                CHECK_RANGE(member_name_id, member_names, "Invalid member name id");
                HKTagTypeMember *member = DA_append_get(&type->members);
                HKTagTypeMember_init(member, &member_names->items[member_name_id]);
                member->flags = member_flags;
                member->offset = member_offset;
                assert(member_unk>0);
                member->type = &types->items[member_unk];
            }
        }

        if (flags & Interfaces) {
            const int64 iface_count = read_compressed_int(buffer);
            DA_reserve(&type->interfaces, iface_count);
            for (int i = 0; i < iface_count; ++i) {
                const int64 iface_type_id = read_compressed_int(buffer);
                const int64 offset = read_compressed_int(buffer);
                HKTagInterface *iface = DA_append_get(&type->interfaces);
                iface->offset = offset;
                iface->type = &types->items[iface_type_id];
            }
        }
        if (flags & Attribute) {
            assert(false);
        }
    }
    return true;
}

bool read_type_hashes(MemoryBuffer *mb, const DynamicArray_HKTagType *types) {
    Buffer *buffer = (Buffer *) mb;
    const int64 count = read_compressed_int(buffer);
    for (int i = 0; i < count; i++) {
        const int64 type_id = read_compressed_int(buffer);
        uint32 hash;
        if (buffer->read_uint32(buffer, &hash) != BUFFER_SUCCESS) {
            printf("[ERROR]: Failed to read type hash\n");
            return false;
        }
        CHECK_RANGE(type_id, types, "Invalid type id in THSH");
        HKTagType *type = &types->items[type_id];
        type->hash = hash;
    }
    return true;
}

#define CHUNK_SLICE(buffer, slice_buffer, tag_name, FAIL_EXIT_LABEL){\
const TagHeader tag_header = expect_tag(buffer, tag_name);\
MemoryBuffer_allocate(slice_buffer, tag_header.size - 8);\
Buffer *body_buffer = (Buffer *) slice_buffer;\
if (buffer->read(buffer, slice_buffer->data, tag_header.size - 8, NULL) != BUFFER_SUCCESS) {\
    printf("[ERROR]: Failed to read TBOD buffer\n");\
    goto FAIL_EXIT_LABEL;\
}\
}
bool TYPETag_from_buffer(TagFile *tf, Buffer *buffer) {
    bool result = true;
    MemoryBuffer *body_slice = MemoryBuffer_new();
    expect_tag(buffer, "TYPE");

    DynamicArray_String class_names = {0};
    DynamicArray_String member_names = {0};

    SkipChunk_from_buffer(buffer);
    const TagHeader tstr_header = expect_tag(buffer, "TSTR");
    Strings_from_buffer(&tstr_header, &class_names, buffer);

    CHUNK_SLICE(buffer, body_slice, "TNAM", FAILED);
    if (!read_type_identity(body_slice, &tf->types, &class_names)) {
        printf("[ERROR]: Failed to read TNAM data\n");
        goto FAILED;
    }

    const TagHeader fstr_header = expect_tag(buffer, "FSTR");
    Strings_from_buffer(&fstr_header, &member_names, buffer);

    CHUNK_SLICE(buffer, body_slice, "TBOD", FAILED);
    if (!read_type_body(body_slice, &tf->types, &member_names)) {
        printf("[ERROR]: Failed to read TBOD data\n");
        goto FAILED;
    }

    CHUNK_SLICE(buffer, body_slice, "THSH", FAILED);
    if (!read_type_hashes(body_slice, &tf->types)) {
        printf("[ERROR]: Failed to read type hashes\n");
        goto FAILED;
    }
    // printf("i, Type Name, flags, size, align, format, Data Type\n");
    // for (uint32 i = type_tag->types.count - 1; i > 0; i--) {
    //     const HKType *type = &type_tag->types.items[i];
    //     // printf as below, but in CSV format
    //     // printf("%i, %s, 0x%08X, %d, %d, %d, %s\n", i, String_data(&type->name), type->flags, type->size, type->align,
    //     // type->format, HKTYPE_NAMES[type->data_type]);
    //     HKType_print(type, stdout, 0);
    //     // printf("Type %d: %s, flags: 0x%08X, size: %d, align: %d, format: %d\n", i, String_data(&type->name), type->flags, type->size, type->align, type->format);
    // }

    SkipChunk_from_buffer(buffer);
    goto EXIT;
FAILED:
    result = false;
EXIT:
    DA_free_with_inner(&class_names, {String_free(it);});
    DA_free_with_inner(&member_names, {String_free(it);});
    body_slice->close(body_slice);
    return result;
}

bool IndexTag_from_buffer(TagFile *tf, Buffer *buffer) {
    TagHeader index_header;
    TagHeader_from_buffer(&index_header, buffer);

    if (memcmp(index_header.ident, "INDX", 4) != 0) {
        printf("[ERROR]: Invalid INDX tag ident: %.4s\n", index_header.ident);
        exit(1);
    }
    MemoryBuffer *mb = MemoryBuffer_new();
    CHUNK_SLICE(buffer, mb, "ITEM", FAILED);
    uint32 approx_count = mb->size / sizeof(HKItem);
    DA_init(&tf->items, HKItem, approx_count);
    while (Buffer_remaining((Buffer *) mb, NULL)) {
        HKItem *item = DA_append_get(&tf->items);
        mb->read(mb, item, sizeof(HKItem), NULL);
    }
    SkipChunk_from_buffer(buffer);
    bool status = true;
    goto EXIT;
FAILED:
    status = false;
EXIT:
    mb->close(mb);
    return status;
}

bool TAG0Tag_from_buffer(TagFile *tf, Buffer *buffer) {
    expect_tag(buffer, "TAG0");

    if (!SDKVTag_from_buffer(tf, buffer)) {
        printf("[ERROR]: Failed to read SDKV tag\n");
        return false;
    }
    if (!DATATag_from_buffer(tf, buffer)) {
        printf("[ERROR]: Failed to read DATA tag\n");
        return false;
    }
    if (!TYPETag_from_buffer(tf, buffer)) {
        printf("[ERROR]: Failed to read TYPE tag\n");
        return false;
    }
    if (!IndexTag_from_buffer(tf, buffer)) {
        printf("[ERROR]: Failed to read INDX tag\n");
        return false;
    }

    DA_FORI(tf->items, i) {
        if (i == 0)continue;
        const HKItem *item = &tf->items.items[i];
        const HKTagType type = tf->types.items[item->type];
        printf("Index Item %d: type: %d (%s), flags: 0x%02X, offset: %d, count: %d\n", i, item->type,
               type.name.buffer, item->flags, item->offset, item->count);
    }

    return true;
}


bool TagFile_from_buffer(TagFile *tf, Buffer *buffer) {
    TAG0Tag_from_buffer(tf, buffer);
    return true;
}

HK_SDKVersion TagFile_get_sdk_version(TagFile *tf) {
    if (memcmp(tf->ver, "2015", 4) == 0) {
        return HK_SDK2015;
    }
    if (memcmp(tf->ver, "2016", 4) == 0) {
        return HK_SDK2016;
    }
    if (memcmp(tf->ver, "2017", 4) == 0) {
        return HK_SDK2017;
    }

    printf("[ERROR]: Unsupported SDK version: %.8s\n", tf->ver);
    exit(1);
}

void TagFile_free(TagFile *tf) {
    DA_free(&tf->data);
    DA_free(&tf->items);
    DA_free_with_inner(&tf->types, {HKTagType_free(it);});
}
