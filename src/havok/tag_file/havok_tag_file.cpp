// Created by RED on 10.10.2025.

#include "havok/tag_file/havok_tag_file.h"

#include <cassert>
#include <cstring>
#include <unordered_map>

#include "redscore/platform/logger.h"
#include "redscore/platform/file/memory_buffer.h"
#include "utils/endian.h"
#include "utils/hash_helper.h"

namespace HavokTypes {
    struct hkContainerHeapAllocator;
}

using namespace Havok::Tag;

static TagHeader expect_tag(std::unique_ptr<IO::File> &buffer, const char expected_ident[5]) {
    TagHeader header(buffer);
    if (std::memcmp(header.ident, expected_ident, 4) != 0) {
        GLog_Error("Expected tag {:.4s} but got {:.4s}", expected_ident, header.ident);
        throw std::runtime_error("Unexpected tag");
    }
    return header;
}

static void skip(const TagHeader &h, const std::unique_ptr<IO::File> &buffer) {
    if (h.size < 8) throw std::runtime_error("Invalid tag size");
    buffer->skip(h.size - 8);
}

static std::unique_ptr<IO::File> slice_tag(const TagHeader &h, const std::unique_ptr<IO::File> &buffer) {
    if (h.size < 8) throw std::runtime_error("Invalid tag size");
    std::vector<uint8> tag_buffer(h.size - 8);
    buffer->read_exact(tag_buffer);
    return std::make_unique<IO::MemoryFile>(std::move(tag_buffer));
}

static uint64 read_u8(const std::unique_ptr<IO::File> &buffer) { return buffer->read_pod<uint8>(); }

int64 read_compressed_int(const std::unique_ptr<IO::File> &buffer) {
    const uint64 b0 = read_u8(buffer);

    if ((b0 & 0x80u) == 0) return static_cast<int64>(b0);

    const uint64 top = b0 >> 3;

    auto read_be = [&](int n) -> uint64 {
        uint64 v = b0;
        for (int i = 0; i < n; ++i) v = (v << 8) | read_u8(buffer);
        return v;
    };

    switch (top) {
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:
            return static_cast<int64>(read_be(1) & 0x3FFFu);

        case 0x18:
        case 0x19:
        case 0x1A:
        case 0x1B:
            return static_cast<int64>(read_be(2) & 0x1FFFFFu);

        case 0x1C:
            return static_cast<int64>(read_be(3) & 0x7FFFFFFu);

        case 0x1D:
            return static_cast<int64>(read_be(4) & 0x7FFFFFFFFull);

        case 0x1E:
            // b0 + 7 bytes
            return static_cast<int64>(read_be(7) & 0x7FFFFFFFFFFFFFFull);

        case 0x1F: {
            const uint64 v6 = b0 & 7u;
            if (v6 == 0) {
                return static_cast<int64>(read_be(5) & 0xFFFFFFFFFFull); // b0 + 5 bytes
            }
            if (v6 == 1) {
                // full 64-bit in next 8 bytes (ignores b0)
                uint64 v = 0;
                for (int i = 0; i < 8; ++i) v = (v << 8) | read_u8(buffer);
                return static_cast<int64>(v);
            }
            return 0;
        }
        default:
            break;
    }

    // Legacy fallback paths you had:
    if ((b0 & 0xC0u) == 0x80u) {
        const uint64 b1 = read_u8(buffer);
        return static_cast<int64>(((b0 & 0x3Fu) << 8) | b1);
    }
    if ((b0 & 0xE0u) == 0xC0u) {
        const uint64 b1 = read_u8(buffer);
        const uint64 b2 = read_u8(buffer);
        return static_cast<int64>(((b0 & 0x1Fu) << 16) | (b1 << 8) | b2);
    }
    if ((b0 & 0xF0u) == 0xE0u) {
        const uint64 b1 = read_u8(buffer);
        const uint64 b2 = read_u8(buffer);
        const uint64 b3 = read_u8(buffer);
        return static_cast<int64>(((b0 & 0x0Fu) << 24) | (b1 << 16) | (b2 << 8) | b3);
    }

    return 0;
}

TagHeader::TagHeader(std::unique_ptr<IO::File> &buffer) {
    auto size_and_flags = buffer->read_pod<uint32>();
    size_and_flags = BE32TOH(size_and_flags);
    size = (size_and_flags & 0x0FFFFFFF);
    flags = size_and_flags >> 28;
    buffer->read(ident, 4);
}

HKItem::HKItem(std::unique_ptr<IO::File> &buffer, const std::vector<SharedType> &types) {
    const auto type_and_flags = buffer->read_pod<uint32>();
    const uint32 type_id = (type_and_flags & 0xFFFFFF);
    type_ = types[type_id];
    flags = type_and_flags >> 24;
    offset = buffer->read_pod<uint32>();
    count = buffer->read_pod<uint32>();
}

SharedType HKItem::type() const {
    if (auto sp = type_.lock()) return sp;
    throw std::runtime_error("Type for item is expired");
}

void HKItem::type(const SharedType &type) {
    type_ = type;
}

TagFile::TagFile(std::unique_ptr<IO::File> &&buffer) {
    expect_tag(buffer, "TAG0");
    read_SDKV_tag(buffer);
    read_DATA_tag(buffer);
    read_TYPE_tag(buffer);
    read_INDX_tag(buffer);
    post_process_types();
}

std::vector<std::string> read_strings(std::unique_ptr<IO::File> &buffer) {
    std::vector<std::string> out;
    while (buffer->remaining() > 0) {
        std::string s;
        buffer->read_cstring(s);
        out.push_back(std::move(s));
    }
    return out;
}

const HKItem &TagFile::get_item_info(uint32 i) {
    if (i >= m_items.size()) {
        GLog_Error("Item index {} out of range", i);
        throw std::runtime_error("Item index out of range");
    }
    return m_items[i];
}

IO::MemoryViewFile TagFile::get_item_buffer(uint32 i) const {
    if (i == 0 || i >= m_items.size()) {
        GLog_Error("Item index {} out of range", i);
        throw std::runtime_error("Item index out of range");
    }
    const auto &item = m_items[i];
    return m_data->take_span(item.type()->size() * item.count, item.offset);
}

void TagFile::read_SDKV_tag(std::unique_ptr<IO::File> &buffer) {
    const auto header = expect_tag(buffer, "SDKV");
    if (header.size - 8 < 8) {
        GLog_Error("Invalid SDKV tag size: {}", header.size);
        throw std::runtime_error("Invalid SDKV tag size");
    }
    buffer->read(m_ver, 8);
}

void TagFile::read_DATA_tag(std::unique_ptr<IO::File> &buffer) {
    const auto header = expect_tag(buffer, "DATA");
    std::vector<uint8> data(header.size - 8);
    buffer->read_exact(data);
    m_data = std::make_unique<IO::MemoryFile>(std::move(data));
}

static void read_type_identities(std::unique_ptr<IO::File> &buffer,
                                 const std::vector<std::string> &names,
                                 std::vector<SharedType> &types) {
    const auto type_count64 = read_compressed_int(buffer);
    if (type_count64 <= 0) throw std::runtime_error("Type count is zero");
    const size_t type_count = static_cast<size_t>(type_count64);

    types.clear();
    types.reserve(type_count);
    types.emplace_back(std::make_shared<Type>()); // id 0 sentinel

    for (size_t i = 1; i < type_count; ++i) {
        types.emplace_back(std::make_shared<Type>(buffer, names));
    }

    buffer->align(4);
    if (buffer->remaining() != 0) {
        GLog_Error("TNAM did not read entire buffer, size={} remaining={}", buffer->get_size(), buffer->remaining());
        throw std::runtime_error("TNAM did not read entire buffer");
    }
}

template<typename T>
const T &check_id_range(const uint64 id, const std::vector<T> &array) {
    if (id >= array.size()) {
        throw std::runtime_error("ID out of range");
    }
    return array[id];
}


void read_type_bodies(std::unique_ptr<IO::File> &buffer, std::vector<std::shared_ptr<Type> > &types,
                      const std::vector<std::string> &member_names) {
    while (buffer->remaining()) {
        const auto type_id = read_compressed_int(buffer);
        if (type_id == 0) {
            break;
        }
        const auto &type = check_id_range(type_id, types);
        if (const auto parent_id = read_compressed_int(buffer); parent_id != 0) {
            type->parent = check_id_range(parent_id, types);
        }
        const auto flags = static_cast<TypeFlags>(read_compressed_int(buffer));
        if (flags & TypeFlags::Format) {
            const auto format = read_compressed_int(buffer);
            type->format = format >> 4;
            type->data_type = static_cast<DataType>(format & 0xF);
        }
        if (flags & TypeFlags::SubType) {
            type->sub_type = read_compressed_int(buffer);
        }
        if (flags & TypeFlags::Version) {
            type->version = read_compressed_int(buffer);
        }
        if (flags & TypeFlags::SizeAlign) {
            type->size(read_compressed_int(buffer));
            type->align(read_compressed_int(buffer) & 0xFF);
        }
        if (flags & TypeFlags::Flags) {
            type->flags = read_compressed_int(buffer);
        }
        if (flags & TypeFlags::Fields) {
            const auto encoded = read_compressed_int(buffer);
            const auto field_count = encoded & 0xffff;
            const auto prop_count = encoded >> 16;
            if (prop_count > 0) {
                GLog_Warning("Unsupported properties in type body, prop count: {}", prop_count);
            }

            type->members.reserve(field_count);
            auto &members = type->members;
            for (uint32 i = 0; i < field_count; i++) {
                const auto member_name_id = read_compressed_int(buffer);
                const auto member_flags = read_compressed_int(buffer);
                const auto member_offset = read_compressed_int(buffer);
                const auto member_type_id = read_compressed_int(buffer);
                members.emplace_back(check_id_range(member_name_id, member_names),
                                     member_flags, member_offset,
                                     check_id_range(member_type_id, types));
            }
        }
        if (flags & TypeFlags::Interfaces) {
            const auto iface_count = read_compressed_int(buffer);
            type->interfaces.reserve(iface_count);
            auto &interfaces = type->interfaces;
            for (uint32 i = 0; i < iface_count; i++) {
                const auto iface_type_id = read_compressed_int(buffer);
                const auto offset = read_compressed_int(buffer);
                interfaces.emplace_back(iface_type_id, offset);
            }
        }
        if (flags & TypeFlags::Attribute) {
            GLog_Error("Unsupported Attribute flag in type body");
            throw std::runtime_error("Unsupported Attribute flag in type body");
        }
    }
}

void read_type_hashes(const std::unique_ptr<IO::File> &buffer, const std::vector<std::shared_ptr<Type> > &types) {
    const auto count = read_compressed_int(buffer);
    for (uint32 i = 0; i < count; i++) {
        const auto type_id = read_compressed_int(buffer);
        const auto hash = buffer->read_pod<uint32>();
        if (type_id == 0 || type_id > types.size()) {
            GLog_Error("Invalid type id in THSH: {}", type_id);
            throw std::runtime_error("Invalid type id in THSH");
        }
        types[type_id]->hash = hash;
    }
}

SDKVersion TagFile::version() const {
    if (!std::memcmp(m_ver, "2015", 4)) return SDKVersion::SDK2015;
    if (!std::memcmp(m_ver, "2016", 4)) return SDKVersion::SDK2016;
    if (!std::memcmp(m_ver, "2017", 4)) return SDKVersion::SDK2017;

    GLog_Error("Unsupported SDK version: {:.8}", m_ver);
    throw std::runtime_error("Unsupported SDK version");
}

void TagFile::read_TYPE_tag(std::unique_ptr<IO::File> &buffer) {
    expect_tag(buffer, "TYPE");
    skip(expect_tag(buffer, "TPTR"), buffer);

    auto strings_buffer = slice_tag(expect_tag(buffer, "TSTR"), buffer);
    const std::vector<std::string> class_names = read_strings(strings_buffer);

    auto type_identity_buffer = slice_tag(expect_tag(buffer, "TNAM"), buffer);
    read_type_identities(type_identity_buffer, class_names, m_types);

    auto member_names_buffer = slice_tag(expect_tag(buffer, "FSTR"), buffer);
    const std::vector<std::string> member_names = read_strings(member_names_buffer);

    auto type_body_buffer = slice_tag(expect_tag(buffer, "TBOD"), buffer);
    read_type_bodies(type_body_buffer, m_types, member_names);

    auto type_hashes_buffer = slice_tag(expect_tag(buffer, "THSH"), buffer);
    read_type_hashes(type_hashes_buffer, m_types);

    skip(expect_tag(buffer, "TPAD"), buffer);
}

void TagFile::read_INDX_tag(std::unique_ptr<IO::File> &buffer) {
    auto index_chunk_buffer = slice_tag(expect_tag(buffer, "INDX"), buffer);
    auto index_items_buffer = slice_tag(expect_tag(index_chunk_buffer, "ITEM"), index_chunk_buffer);
    const auto item_count = index_items_buffer->get_size() / 12;
    m_items.reserve(item_count);
    for (uint32 i = 0; i < item_count; i++) {
        m_items.emplace_back(index_items_buffer, m_types);
    }
    skip(expect_tag(index_chunk_buffer, "PTCH"), index_chunk_buffer);
}

void TagFile::post_process_types() {
    // Pass 0: resolve template arg ids -> WeakType
    for (const auto &t: m_types) {
        if (!t) continue;
        for (auto &ta: t->template_args) {
            if (ta.name.empty()) continue;
            if (ta.name[0] == 't') {
                const auto type_id = std::get<int64>(ta.value);
                if (type_id <= 0 || type_id >= static_cast<int64>(m_types.size())) {
                    GLog_Error("Invalid template argument type id: {}", type_id);
                    throw std::runtime_error("Invalid template argument type id");
                }
                ta.value = WeakType{m_types[static_cast<size_t>(type_id)]};
            }
        }
    }

    // Canonicalize by unique_id
    std::unordered_map<std::string, SharedType> canonical;
    canonical.reserve(m_types.size());

    std::unordered_map<SharedType, SharedType> remap;
    remap.reserve(m_types.size());

    std::vector<SharedType> unique;
    unique.reserve(m_types.size());

    for (auto &t: m_types) {
        if (!t) continue;

        auto [it, inserted] = canonical.emplace(t->unique_id(), t);
        if (inserted) {
            unique.push_back(t);
            continue;
        }

        SharedType &keep = it->second;

        if (!t->members.empty() && keep->members.empty()) {
            keep->members = t->members;
        }
        if (!t->template_args.empty() && keep->template_args.empty()) {
            keep->template_args = t->template_args;
        }

        remap.emplace(t, keep);
    }

    auto resolve_shared = [&](SharedType p) -> SharedType {
        while (p) {
            auto it = remap.find(p);
            if (it == remap.end() || it->second == p) break;
            p = it->second;
        }
        return p;
    };

    auto resolve_weak_inplace = [&](WeakType &w) {
        if (auto sp = w.lock()) {
            auto rsp = resolve_shared(sp);
            if (rsp != sp) w = WeakType{rsp};
        }
    };

    // Rewrite all references in canonical set
    for (const auto &t: unique) {
        if (!t) continue;

        for (auto &ta: t->template_args) {
            if (std::holds_alternative<WeakType>(ta.value)) {
                resolve_weak_inplace(std::get<WeakType>(ta.value));
            }
        }

        for (auto &m: t->members) {
            auto mt = m.type();
            auto rmt = resolve_shared(mt);
            if (rmt != mt) m.type(rmt);
        }

        if (t->parent) {
            auto rp = resolve_shared(t->parent);
            if (rp != t->parent) t->parent = rp;
        }
    }

    // Rewrite HKItem refs
    for (auto &item: m_items) {
        auto t = item.type();
        auto rt = resolve_shared(t);
        if (rt != t) item.type(rt);
    }

    m_types = std::move(unique);
}
