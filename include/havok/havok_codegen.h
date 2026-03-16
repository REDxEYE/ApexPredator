// Created by RED on 12.10.2025.

#ifndef APEXPREDATOR_HAVOK_CODEGEN_H
#define APEXPREDATOR_HAVOK_CODEGEN_H

#include <filesystem>

#include "havok_types.h"

namespace Havok::CodeGen {
    void generate_code(const TypeLibrary &lib,
                       const std::filesystem::path &sources_path,
                       const std::filesystem::path &headers_path);
}

#endif //APEXPREDATOR_HAVOK_CODEGEN_H
