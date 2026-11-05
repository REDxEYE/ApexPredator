#pragma once

#include "platform/app_state.h"

#include <memory>
#include <optional>
#include <string_view>

// Disengaged: unrecognized payload. Engaged null node: handled without scene output.
// Each game module links one implementation; recognized handlers may consume buffer.
std::optional<VM::NodePtr> dispatch_file(ApexAppState &app_state, uint64 hash,
                                         std::unique_ptr<IO::File> &buffer, std::string_view path);
