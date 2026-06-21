// Created by RED on 18.09.2025.

#ifndef APEXPREDATOR_TAB_ARCHIVE_H
#define APEXPREDATOR_TAB_ARCHIVE_H

#include "unordered_map"
#include "filesystem"


#include "apex/package/tab.h"
#include "platform/archive_manager.h"
#include "redscore/platform/archive.h"
#include "redscore/platform/file/native_file.h"

std::filesystem::path inline get_arc_path(const std::filesystem::path &tab_path) {
    std::filesystem::path arc_path = tab_path;
    arc_path.replace_extension("arc");
    return arc_path;
}

class TabArchive :public Archive<u64> {
public:
    explicit TabArchive(const std::filesystem::path &path): arc_buffer(get_arc_path(path), std::ios::in | std::ios::binary) {
        m_tab_path = path;

        const auto base = m_tab_path.parent_path().parent_path();
        const auto relative_path = std::filesystem::relative(m_tab_path, base);
        m_name = relative_path.string();
        m_hash = hash_string(m_name);

        initialize();
    }

    bool has(std::string_view path);

    bool has(const u64& hash) override;

    std::unique_ptr<IO::File> get(std::string_view path);

    std::unique_ptr<IO::File> get(const u64& hash) override;

    // void all_entries(std::vector<ArchiveEntry> &entries) const override;

    [[nodiscard]] std::string_view name() const override;

    const u64& key() const override;

    static void mount_folder(ArchiveManager<u64>& manager, const std::filesystem::path& path);

    bool foreach_file(const std::function<bool(const ArchiveEntry &)> &callback) override;

private:

    void initialize();

    std::filesystem::path m_tab_path;
    std::string m_name;
    u64 m_hash;
    IO::NativeFile arc_buffer;
    std::unordered_map<uint64, TabEntry> m_entries{};
};




#endif //APEXPREDATOR_TAB_ARCHIVE_H
