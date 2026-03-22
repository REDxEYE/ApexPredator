// Created by RED on 02.10.2025.

#ifndef APEXPREDATOR_SARC_H
#define APEXPREDATOR_SARC_H
#include <ranges>

#include "int_def.h"
#include "redscore/platform/archive.h"
#include "redscore/platform/file/file.h"
#include "utils/hash_helper.h"

struct SArcHeader {
    uint32 version;
    char ident[4];
    uint32 version2;
    uint32 dir_block_len;
};

struct SArcEntry {
    std::string_view name;
    uint32 offset;
    uint32 size;
    uint32 hash;
    uint32 ext_hash;
};


class SArchive : public Archive {
public:
    SArchive(uint32 m_hash, std::unique_ptr<IO::File> buffer);

    [[nodiscard]] bool has_file(std::string_view path) override;

    [[nodiscard]] bool has_file(uint32 hash) override;

    std::unique_ptr<IO::File> get_file(std::string_view path) override;

    std::unique_ptr<IO::File> get_file(uint32 hash) override;

    void all_entries(std::vector<ArchiveEntry> &entries) const override;

    [[nodiscard]] std::string get_name() const override;

    uint32 hash() override;

    [[nodiscard]] auto entries() const {
        return m_entries|std::views::values;
    }

private:
    SArcHeader m_header{};
    uint32 m_hash;
    std::vector<char> m_strings;
    std::unordered_map<uint32, SArcEntry> m_entries;
    std::unique_ptr<IO::File> m_buffer;
};

#endif //APEXPREDATOR_SARC_H
