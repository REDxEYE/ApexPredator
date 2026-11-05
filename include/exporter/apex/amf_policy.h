#pragma once

#include <memory>
#include <string_view>
#include <utility>

#include "exporter/amf_export.h"
#include "redscore/platform/file/file.h"

void normalize_amf_weight_sets(std::vector<VM::VertexAttribute> &attributes);

// Each module links one geometry family and one material policy.
std::pair<uint64, std::unique_ptr<IO::File>> open_amf_model_mesh(
    ApexAppState &app_state, const ADFTypes::AmfModel &model);

void export_amf_material(ApexAppState &app_state, const VM::NodePtr &model_root,
                         const ADFTypes::AmfMaterial &material,
                         std::string_view material_name, std::string_view render_block_id);
