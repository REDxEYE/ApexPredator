// Created by RED on 12.01.2026.

#ifndef APEXPREDATOR_HAVOK_EXPORT_H
#define APEXPREDATOR_HAVOK_EXPORT_H
#include <cstdint>
#include <memory>
#include <string_view>
#include "glm/glm.hpp"
#include "havok/generated/havok_types.h"
#include "platform/app_state.h"

inline constexpr std::uint8_t HAVOK_MAGIC[] = {'T','A','G','0'};


VM::NodePtr export_havok_file(ApexAppState& app_state, std::unique_ptr<IO::File> && buffer, std::string_view path);

glm::mat4 build_matrix(const HavokTypes::hkQsTransform& transform);

VM::NodePtr export_skeleton(ApexAppState& app_state, const HavokTypes::hkaSkeleton *skeleton);

#endif //APEXPREDATOR_HAVOK_EXPORT_H