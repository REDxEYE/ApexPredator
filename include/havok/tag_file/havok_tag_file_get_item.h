// Created by RED on 03.03.2026.
#pragma once

#include <memory>

#include "havok/havok_base_type.h"

namespace Havok::Tag {
    std::unique_ptr<BaseType> get_item(TagFile& tag_file, uint32 i);
}
