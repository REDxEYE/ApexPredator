#include "exporter/apex/amf_material_merged.h"

void export_amf_material(ApexAppState &app_state, const VM::NodePtr &,
                         const ADFTypes::AmfMaterial &material,
                         std::string_view material_name, std::string_view render_block_id) {
    export_amf_merged_material(app_state, material, material_name, render_block_id);
}
