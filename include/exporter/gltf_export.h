#pragma once

#include <cstdint>

class ApexAppState;

// Calls the format dispatcher linked by the selected game module.
void normal_export(ApexAppState &app_state, std::uint64_t asset_hash);
