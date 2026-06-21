// Created by RED on 02.10.2025.

#ifndef APEXPREDATOR_SARC_H
#define APEXPREDATOR_SARC_H
#include <ranges>
#include <unordered_map>

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


class SArchive : public Archive<u64> {
public:
    SArchive(uint64 m_hash, std::unique_ptr<IO::File> buffer);

    [[nodiscard]] bool has(std::string_view path);

    [[nodiscard]] bool has(const u64& hash) override;

    std::unique_ptr<IO::File> get(std::string_view path);

    std::unique_ptr<IO::File> get(const u64& hash) override;

    // void all_entries(std::vector<ArchiveEntry> &entries) const override;

    [[nodiscard]] std::string_view name() const override;

    [[nodiscard]] const u64& key() const override;

    [[nodiscard]] auto entries() const {
        return m_entries|std::views::values;
    }

    bool foreach_file(const std::function<bool(const ArchiveEntry &)> &callback) override;

private:
    SArcHeader m_header{};
    uint64 m_hash;
    std::string m_name;
    std::vector<char> m_strings;
    std::unordered_map<uint64, SArcEntry> m_entries;
    std::unique_ptr<IO::File> m_buffer;
};

#endif //APEXPREDATOR_SARC_H
