#include "exporter/apex/amf_policy.h"

#include <cstring>
#include <vector>

// A vertex's influences span every WEIGHTS_n stream, not each group of four independently.
void normalize_amf_weight_sets(std::vector<VM::VertexAttribute> &attributes) {
    std::vector<VM::VertexAttribute *> weights;
    for (auto &attribute: attributes)
        if (attribute.usage == VM::ElementUsage::Weights) weights.push_back(&attribute);
    if (weights.empty()) return;

    for (size_t vertex = 0; vertex < weights.front()->count; ++vertex) {
        float total = 0.f;
        for (const auto *attribute: weights) {
            float values[4];
            std::memcpy(values, attribute->data.data() + vertex * sizeof(values), sizeof(values));
            for (float value: values) total += value;
        }
        if (total <= 0.f) continue;
        for (auto *attribute: weights) {
            float values[4];
            auto *data = attribute->data.data() + vertex * sizeof(values);
            std::memcpy(values, data, sizeof(values));
            for (float &value: values) value /= total;
            std::memcpy(data, values, sizeof(values));
        }
    }
}
