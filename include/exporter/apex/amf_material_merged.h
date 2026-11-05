#pragma once

#include "exporter/apex/amf_policy.h"

// Shared GeneralR2/Character PBR slots for Rage2 and Second Extinction.
void export_amf_merged_material(ApexAppState &app_state,
                                const ADFTypes::AmfMaterial &material,
                                std::string_view material_name, std::string_view render_block_id);
