// Created by RED on 18.09.2025.

#ifndef APEXPREDATOR_TAB_ARCHIVE_H
#define APEXPREDATOR_TAB_ARCHIVE_H

#include "unordered_map"
#include "filesystem"


#include "apex/package/tab.h"
#include "platform/archive.h"
#include "platform/archive_manager.h"
#include "platform/file/file_buffer.h"

std::filesystem::path inline get_arc_path(const std::filesystem::path &tab_path) {
    std::filesystem::path arc_path = tab_path;
    arc_path.replace_extension("arc");
    return arc_path;
}

class TabArchive :public Archive {
public:
    explicit TabArchive(const std::filesystem::path &path): arc_buffer(get_arc_path(path), std::ios::in | std::ios::binary) {
        m_tab_path = path;
        initialize();
    }

    bool has_file(std::string_view path) const override;

    bool has_file(uint32 hash) const override;

    std::unique_ptr<IO::File> get_file(std::string_view path) override;

    std::unique_ptr<IO::File> get_file(uint32 hash) override;

    void all_entries(std::vector<ArchiveEntry> &entries) const override;

    [[nodiscard]] std::string get_name() const override;

    uint32 hash() override;

    static void mount_folder(ArchiveManager& manager, const std::filesystem::path& path);

private:

    void initialize();

    std::filesystem::path m_tab_path;
    IO::NativeFile arc_buffer;
    std::unordered_map<uint32, TabEntry> m_entries{};
};




#endif //APEXPREDATOR_TAB_ARCHIVE_H
