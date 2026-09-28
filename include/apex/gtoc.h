#pragma once


#include <string_view>
#include <vector>

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

class GTOCArchive : public Archive<uint64> {
public:
    GTOCArchive(ArchiveManager<uint64> &m_manager, std::unique_ptr<IO::File> buffer, uint64 hash)
        : m_manager(m_manager),
          m_file(*buffer) {
        for (const auto &file: m_file.files()) {
            uint64 asset_hash = asset_path_hash(file.name);
            m_hash_remap.emplace(asset_hash, file.hash);
        }
        m_hash = hash;
        m_name = std::format("GTOC{:016X}", hash);
    }

    [[nodiscard]] bool has(const uint64 &key) override;

    std::unique_ptr<IO::File> get(const uint64 &key) override;

    [[nodiscard]] std::string_view name() const override;

    [[nodiscard]] const uint64 &key() const override;

    bool foreach_file(const std::function<bool(const ArchiveEntry &)> &callback) override;

private:
    ArchiveManager<uint64> &m_manager;
    std::unordered_map<uint64, uint32> m_hash_remap;
    GTOCFile m_file;
    uint64 m_hash;
    std::string m_name;
};
