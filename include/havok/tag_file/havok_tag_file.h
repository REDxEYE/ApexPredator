// Created by RED on 10.10.2025.

#pragma once

#include "havok/havok_base_type.h"
#include "havok/tag_file/havok_tag_types.h"
#include "platform/file/file.h"
#include "platform/file/memory_buffer.h"
#include "int_def.h"

namespace Havok {
    struct BaseType;
}

namespace Havok::Tag {
    struct TagHeader {
        explicit TagHeader(std::unique_ptr<IO::File> &buffer);

        uint32 size;
        uint8 flags;
        char ident[4]{0, 0, 0, 0};
    };

    enum class SDKVersion {
        SDK2015,
        SDK2016,
        SDK2017,
    };


    struct HKItem {
        uint8 flags;
        uint32 offset;
        uint32 count;

        HKItem(std::unique_ptr<IO::File> &buffer, const std::vector<SharedType> &types);

        [[nodiscard]] SharedType type() const;

        void type(const SharedType &);

    private:
        WeakType type_;
    };


    class TagFile {
    public:
        explicit TagFile(std::unique_ptr<IO::File> &&buffer);

        [[nodiscard]] SDKVersion version() const;

        [[nodiscard]] const std::unique_ptr<IO::MemoryFile> &data() const {
            return m_data;
        }

        [[nodiscard]] const std::vector<SharedType> &types() const {
            return m_types;
        }

        [[nodiscard]] const std::vector<HKItem> &items() const {
            return m_items;
        }


        const HKItem &get_item_info(uint32 i);

        [[nodiscard]] IO::MemoryViewFile get_item_buffer(uint32 i) const;

    private:
        void read_SDKV_tag(std::unique_ptr<IO::File> &buffer);

        void read_DATA_tag(std::unique_ptr<IO::File> &buffer);

        void read_TYPE_tag(std::unique_ptr<IO::File> &buffer);

        void read_INDX_tag(std::unique_ptr<IO::File> &buffer);

        void post_process_types();


        char m_ver[9]{};
        std::unique_ptr<IO::MemoryFile> m_data;
        std::vector<SharedType> m_types;
        std::vector<HKItem> m_items;
    };
}
