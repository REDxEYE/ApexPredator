#pragma once

#include "apex/pcbb/pcbb.hpp"
#include "platform/app_state.h"

namespace Apex {
    VM::NodePtr export_pcbb(ApexAppState &app_state, uint64 path_hash, const std::unique_ptr<IO::File> &&buffer);
}
