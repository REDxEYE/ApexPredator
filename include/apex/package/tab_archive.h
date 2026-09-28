// Created by RED on 18.09.2025.

#ifndef APEXPREDATOR_TAB_ARCHIVE_H
#define APEXPREDATOR_TAB_ARCHIVE_H

#include "filesystem"

#include "apex/package/tab.hpp"
#include "platform/archive_manager.h"
#include "redscore/platform/archive.h"
#include "redscore/platform/file/native_file.h"



class TabArchive : public Archive<u64> {
public:
    explicit TabArchive(const std::filesystem::path &path) {
        m_tab_path = path;

        const auto base = m_tab_path.parent_path().parent_path();
        const auto relative_path = std::filesystem::relative(m_tab_path, base);
        m_name = relative_path.string();
        m_hash = asset_path_hash(m_name);

        initialize();
    }

    ~TabArchive() override {
        _impl.reset();
    };

    bool has(std::string_view path);

    bool has(const u64 &hash) override;

    std::unique_ptr<IO::File> get(std::string_view path);

    std::unique_ptr<IO::File> get(const u64 &hash) override;

    [[nodiscard]] std::string_view name() const override;

    [[nodiscard]] const u64 &key() const override;

    static void mount_folder(ArchiveManager<u64> &manager, const std::filesystem::path &path);
    static void mount_folder_optional(ArchiveManager<u64> &manager, const std::filesystem::path &path);


    bool foreach_file(const std::function<bool(const ArchiveEntry &)> &callback) override;

private:
    void initialize();

    std::unique_ptr<Tab> _impl{nullptr};

    std::filesystem::path m_tab_path;
    std::string m_name;
    u64 m_hash;
};


#endif //APEXPREDATOR_TAB_ARCHIVE_H
