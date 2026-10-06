#pragma once


#include <string_view>
#include <vector>

#include "hashes.h"
#include "redscore/int_def.h"
#include "redscore/platform/archive.h"
#include "redscore/platform/archive_manager.h"
#include "redscore/platform/file/file.h"
#include "utils/hash_helper.h"

inline constexpr char GTOC_MAGIC[] = "GT0C";

struct GTOCFileEntry {
    uint32 hash;
    uint32 ext_hash;
    uint32 size;
    std::string_view name;
};

struct GTOCMember {
    uint32 file_index;
    uint32 offset;
};

struct GTOCArchiveEntry {
    uint32 hash;
    uint32 tag;
    std::vector<GTOCMember> members;
};

class GTOCFile {
public:
    explicit GTOCFile(IO::File &buffer);

    GTOCFile(const GTOCFile &) = delete;

    GTOCFile &operator=(const GTOCFile &) = delete;

    GTOCFile(GTOCFile &&) noexcept = default;

    GTOCFile &operator=(GTOCFile &&) noexcept = default;

    [[nodiscard]] const std::vector<GTOCFileEntry> &files() const { return m_files; }
    [[nodiscard]] const std::vector<GTOCArchiveEntry> &archives() const { return m_archives; }

private:
    // Names refer into this owned buffer, whose allocation survives moves.
    std::vector<uint8> m_data;
    std::vector<GTOCFileEntry> m_files;
    std::vector<GTOCArchiveEntry> m_archives;
};

class GTOCArchive : public Archive<u64> {
public:
    GTOCArchive(ArchiveManager<u64> &m_manager, std::unique_ptr<IO::File> buffer, u64 hash)
        : m_manager(m_manager),
          m_file(*buffer) {
        for (const auto &file: m_file.files()) {
            u64 asset_hash = asset_path_hash(file.name);
            m_hash_remap.emplace(asset_hash, file.hash);
        }
        m_hash = hash;
        auto known_name = find_asset_name(hash).value_or(std::format("GTOC{:016X}", hash));
        m_name = known_name;
    }

    [[nodiscard]] bool has(const u64 &key) override;

    std::unique_ptr<IO::File> get(const u64 &key) override;

    [[nodiscard]] std::string_view name() const override;

    [[nodiscard]] const u64 &key() const override;

    [[nodiscard]] const u64 &get_parent_key() override;

    bool foreach_file(const std::function<bool(const ArchiveEntry &)> &callback) override;

    const GTOCFile &toc() const { return m_file; }

private:
    ArchiveManager<u64> &m_manager;
    std::unordered_map<u64, uint32> m_hash_remap;
    GTOCFile m_file;
    u64 m_hash;
    std::string m_name;
};
