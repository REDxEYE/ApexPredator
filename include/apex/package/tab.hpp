// Created by RED on 17.09.2025.

#pragma once

#include <memory>

#include "int_def.h"
#include "platform/archive_manager.h"
#include "redscore/platform/archive.h"
#include "redscore/platform/file/native_file.h"


class Tab {
public:
    virtual ~Tab() = default;

    virtual bool has(std::string_view path) = 0;

    virtual bool has(const u64 &hash) = 0;

    virtual std::unique_ptr<IO::File> get(std::string_view path) = 0;

    virtual std::unique_ptr<IO::File> get(const u64 &hash) = 0;

    virtual const u64 &key() const =0;

    virtual bool foreach_file(const std::function<bool(const Archive<u64>::ArchiveEntry &)> &callback) = 0;

    std::string m_name;
    u64 m_hash;
};

std::filesystem::path inline get_arc_path(const std::filesystem::path &tab_path) {
    std::filesystem::path arc_path = tab_path;
    arc_path.replace_extension("arc");
    return arc_path;
}
