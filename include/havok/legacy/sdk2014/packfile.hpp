//
// Created by red_eye on 10/12/26.
//

#pragma once
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <stdexcept>
#include <optional>
#include <unordered_map>
#include <vector>

namespace Havok2014 {
    enum class ClassKind;
}

constexpr uint8_t HAVOK2014_MAGIC[] = "\x57\xe0\xe0\x57\x10\xc0\xc0\x10";

namespace HavokTypes {
    extern std::unordered_map<std::string_view, Havok2014::ClassKind> &havok_type_info();
}

namespace Havok2014 {
    [[noreturn]] void invalid(const char *reason);

    enum class ClassKind { root, animationContainer, skeleton };


    struct Location {
        size_t section;
        size_t offset;

        bool operator==(const Location &) const = default;
    };

    struct LocationHash {
        size_t operator()(const Location at) const {
            return std::hash<size_t>{}(at.section) ^ (std::hash<size_t>{}(at.offset) << 1);
        }
    };

    struct Section {
        std::string_view name;
        size_t start = 0;
        size_t data_size = 0;
        std::unordered_map<size_t, Location> pointers;
        std::unordered_map<size_t, std::string_view> virtual_classes;
    };


    class Packfile {
    public:
        explicit Packfile(std::span<const std::uint8_t> bytes);

        [[nodiscard]] Location root() const { return root_; }

        [[nodiscard]] Location field(Location base, size_t offset, size_t length) const;

        void require(Location at, size_t length) const;

        [[nodiscard]] uint8_t u8(const Location at) const {
            require(at, 1);
            return bytes_[absolute(at)];
        }

        [[nodiscard]] uint16_t u16(const Location at) const {
            require(at, 2);
            return raw16(absolute(at));
        }

        [[nodiscard]] int16_t i16(const Location at) const { return std::bit_cast<int16_t>(u16(at)); }

        [[nodiscard]] uint32_t u32(const Location at) const {
            require(at, 4);
            return raw32(absolute(at));
        }

        [[nodiscard]] float f32(const Location at) const { return std::bit_cast<float>(u32(at)); }

        [[nodiscard]] std::optional<Location> pointer(const Location slot) const {
            require(slot, 8);
            if (u32(slot) || u32(field(slot, 4, 4))) invalid("unrelocated pointer data");
            const auto &pointers = sections_[slot.section].pointers;
            const auto it = pointers.find(slot.offset);
            if (it != pointers.end()) return it->second;
            return std::nullopt;
        }

        [[nodiscard]] Location required_pointer(const Location slot) const {
            const auto destination = pointer(slot);
            if (!destination) invalid("missing required pointer fixup");
            return *destination;
        }

        [[nodiscard]] std::string_view string(const Location at) const {
            require(at, 1);
            const auto &section = sections_[at.section];
            const char *begin = reinterpret_cast<const char *>(bytes_.data() + absolute(at));
            const size_t remaining = section.data_size - at.offset;
            const void *end = std::memchr(begin, '\0', remaining);
            if (!end) invalid("unterminated string");
            return {begin, static_cast<size_t>(static_cast<const char *>(end) - begin)};
        }

        struct Array {
            std::optional<Location> data;
            size_t size{};
        };

        [[nodiscard]] Array array(const Location at, const size_t stride) const {
            require(at, 16);
            const size_t count = u32(field(at, 8, 4));
            const size_t capacity = u32(field(at, 12, 4)) & 0x3fffffff;
            if (count > capacity) invalid("array exceeds capacity");
            auto target = pointer(at);
            if (count && !target) invalid("nonempty array missing fixup");
            if (target) {
                if (count > std::numeric_limits<size_t>::max() / stride)
                    invalid("array size overflows");
                require(*target, count * stride);
            }
            return {target, count};
        }

        [[nodiscard]] Location element(const Array &array, const size_t index, const size_t stride) const {
            if (!array.data || index >= array.size) invalid("invalid array element");
            return field(*array.data, index * stride, stride);
        }

        void expect_class(const Location at, const ClassKind kind) const {
            require(at, 8);
            const auto &virtual_classes = sections_[at.section].virtual_classes;
            const auto it = virtual_classes.find(at.offset);
            if (it == virtual_classes.end() || class_kind(it->second) != kind)
                invalid("unexpected or missing virtual class");
        }

        static std::optional<ClassKind> class_kind(const std::string_view name) {
            const auto &registered = HavokTypes::havok_type_info();
            const auto it = registered.find(name);
            if (it == registered.end()) return std::nullopt;
            return it->second;
        }

    private:
        std::span<const uint8_t> bytes_;
        std::vector<Section> sections_;
        Location root_{};

        [[nodiscard]] size_t absolute(const Location at) const { return sections_[at.section].start + at.offset; }

        [[nodiscard]] uint16_t raw16(const size_t pos) const {
            return uint16_t(bytes_[pos]) | uint16_t(bytes_[pos + 1]) << 8;
        }

        [[nodiscard]] uint32_t raw32(const size_t pos) const {
            return uint32_t(bytes_[pos]) | uint32_t(bytes_[pos + 1]) << 8 |
                   uint32_t(bytes_[pos + 2]) << 16 | uint32_t(bytes_[pos + 3]) << 24;
        }

        template<typename F>
        void read_fixups(const size_t start, const size_t end, const size_t stride, const Section &section, F &&visit) {
            size_t offset = start;
            while (end - offset >= stride) {
                const size_t pos = section.start + offset;
                if (raw32(pos) != 0xffffffff) visit(pos);
                else
                    for (size_t j = 0; j < stride; ++j)
                        if (bytes_[pos + j] != 0xff) invalid("malformed fixup padding");
                offset += stride;
            }
            for (; offset < end; ++offset)
                if (bytes_[section.start + offset] != 0xff)
                    invalid("malformed fixup padding");
        }

        void add_pointer(const size_t source_section, size_t source, const size_t target_section, const size_t target) {
            if (target_section >= sections_.size()) invalid("invalid fixup target section");
            require({source_section, source}, 8);
            require({target_section, target}, 1);
            if (!sections_[source_section].pointers.emplace(source, Location{target_section, target}).second)
                invalid("duplicate pointer fixup");
        }
    };
}
