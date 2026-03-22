// Created by RED on 01.10.2025.

#ifndef APEXPREDATOR_AAF_H
#define APEXPREDATOR_AAF_H
#include <memory>
#include <vector>

#include "int_def.h"
#include "redscore/platform/file/file.h"

#define AAF_MAGIC "AAF\0"

struct AAFHeader {
    char ident[4];
    uint32 version;
    char awesome[28];
    uint32 uncompressed_size;
    uint32 section_size;
    uint32 section_count;
};

struct AAFSectionHeader {
    uint32 compressed_size;
    uint32 uncompressed_size;
    uint32 total_size;
    char magic[4];
};

struct AAFSection {
    AAFSectionHeader header;
    std::vector<uint8> buffer;
};


class AAFArchive {
public:
    explicit AAFArchive(std::unique_ptr<IO::File>buffer);

    std::unique_ptr<IO::File> get_data();
private:
    AAFHeader m_header{};
    std::unique_ptr<IO::File> m_buffer;
    std::vector<AAFSection> m_sections;
};


#endif //APEXPREDATOR_AAF_H
