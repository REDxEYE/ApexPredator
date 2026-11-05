//
// Created by red_eye on 10/12/26.
//

#include "havok/legacy/sdk2014/packfile.hpp"


void Havok2014::invalid(const char *reason) {
    throw std::runtime_error(std::string("Invalid Havok 2013 skeleton packfile: ") + reason);
}

Havok2014::Packfile::Packfile(std::span<const std::uint8_t> bytes) : bytes_(bytes) {
    if (bytes.size() < 64 || std::memcmp(bytes.data(), "\x57\xe0\xe0\x57\x10\xc0\xc0\x10", 8) != 0)
        invalid("missing Havok packfile magic");
    if (raw32(12) != 11 || bytes[16] != 8 || bytes[17] != 1 || bytes[19] != 1)
        invalid("unsupported version, pointer width, byte order or layout");
    if (std::string_view(reinterpret_cast<const char *>(bytes.data() + 40), 14) != "hk_2013.2.0-r1" || bytes[54]
        != 0)
        invalid("unsupported Havok contents version");
    if (raw16(62) != 0xffff && raw16(62) != 0)
        invalid("unsupported extended header");

    const auto count = raw32(20);
    if (count == 0 || count > 64 || count > (bytes.size() - 64) / 64)
        invalid("invalid section table");
    const auto contents = raw32(24);
    const auto classname_section = raw32(32);
    if (contents >= count || classname_section >= count)
        invalid("invalid header section index");
    sections_.reserve(count);
    struct Ranges {
        size_t local, global, virtual_, exports, end;
    };
    std::vector<Ranges> ranges;
    ranges.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        const size_t pos = 64 + i * 64;
        if (bytes[pos + 19] != 0xff) invalid("malformed section tag");
        const auto tag = std::string_view(reinterpret_cast<const char *>(bytes.data() + pos), 19);
        const auto tag_end = tag.find('\0');
        if (tag_end == std::string_view::npos) invalid("unterminated section tag");
        const size_t start = raw32(pos + 20);
        const size_t local = raw32(pos + 24);
        const size_t global = raw32(pos + 28);
        const size_t virtual_ = raw32(pos + 32);
        const size_t exports = raw32(pos + 36);
        const size_t imports = raw32(pos + 40);
        const size_t end = raw32(pos + 44);
        if (start > bytes.size() || end > bytes.size() - start ||
            local > global || global > virtual_ || virtual_ > exports ||
            exports > imports || imports > end)
            invalid("invalid section ranges");
        sections_.push_back({tag.substr(0, tag_end), start, local, {}, {}});
        ranges.push_back({local, global, virtual_, exports, end});
    }
    if (sections_[contents].name != "__data__" || sections_[classname_section].name != "__classnames__")
        invalid("unexpected contents or classnames section");
    root_ = {contents, raw32(28)};
    require(root_, 16);
    const Location root_class{classname_section, raw32(36)};
    if (class_kind(string(root_class)) != ClassKind::root)
        invalid("unsupported root class");

    for (size_t i = 0; i < count; ++i) {
        auto &section = sections_[i];
        const auto &r = ranges[i];
        read_fixups(r.local, r.global, 8, section, [&](size_t p) {
            add_pointer(i, raw32(p), i, raw32(p + 4));
        });
        read_fixups(r.global, r.virtual_, 12, section, [&](size_t p) {
            add_pointer(i, raw32(p), raw32(p + 4), raw32(p + 8));
        });
        read_fixups(r.virtual_, r.exports, 12, section, [&](size_t p) {
            const size_t source = raw32(p);
            const size_t class_section = raw32(p + 4);
            const size_t class_offset = raw32(p + 8);
            if (class_section >= sections_.size()) invalid("invalid virtual class section");
            require({i, source}, 8);
            const auto class_name = string({class_section, class_offset});
            if (!section.virtual_classes.emplace(source, class_name).second)
                invalid("duplicate virtual class entry");
        });
    }
    expect_class(root_, ClassKind::root);
}

Havok2014::Location Havok2014::Packfile::field(const Location base, const size_t offset, const size_t length) const {
    if (base.section >= sections_.size()) invalid("invalid section index");
    const auto &section = sections_[base.section];
    if (base.offset > section.data_size || offset > section.data_size - base.offset)
        invalid("object field out of bounds");
    Location result{base.section, base.offset + offset};
    require(result, length);
    return result;
}

void Havok2014::Packfile::require(const Location at, const size_t length) const {
    if (at.section >= sections_.size()) invalid("invalid section index");
    const auto &section = sections_[at.section];
    if (at.offset > section.data_size || length > section.data_size - at.offset)
        invalid("object exceeds section data");
}
