// Created by RED on 02.10.2025.

#ifndef APEXPREDATOR_ARCHIVE_H
#define APEXPREDATOR_ARCHIVE_H
#include <memory>
#include <functional>

#include "int_def.h"
#include "platform/file/memory_buffer.h"

struct ArchiveEntry {
    uint32 path_hash;
    uint32 size;
};


class Archive {
public:
    virtual ~Archive() = default;

    [[nodiscard]] virtual bool has_file(std::string_view path) const = 0;

    [[nodiscard]] virtual bool has_file(uint32 hash) const = 0;

    virtual std::unique_ptr<IO::File> get_file(std::string_view path) = 0;

    virtual std::unique_ptr<IO::File> get_file(uint32 hash) = 0;

    virtual void all_entries(std::vector<ArchiveEntry> &entries) const = 0;

    [[nodiscard]] virtual std::string get_name() const = 0;

    virtual uint32 hash() = 0;

    bool foreach_file(const std::function<bool (const ArchiveEntry &)> &callback) const {
        std::vector<ArchiveEntry> entries;
        all_entries(entries);
        for (const auto &entry: entries) {
            if (!callback(entry)) {
                break;
            }
        }
        return true;
    }
};

#endif //APEXPREDATOR_ARCHIVE_H
