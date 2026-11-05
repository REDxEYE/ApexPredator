#pragma once

#include "exporter/havok_export.h"

void export_animation(ApexAppState &app_state, const HavokTypes::hkaAnimationBinding *binding,
                      const HavokTypes::hkaSkeleton *skeleton, std::string_view animation_name,
                      bool apply_root_motion);
