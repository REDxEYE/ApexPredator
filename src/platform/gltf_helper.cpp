// Created by RED on 08.03.2026.

#include "../../include/platform/gltf_helper.h"

#include <algorithm>
#include <ranges>

#include "glm/gtx/matrix_decompose.hpp"

namespace {
    size_t component_size_bytes(const int component_type) {
        switch (component_type) {
            case TINYGLTF_COMPONENT_TYPE_BYTE:
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                return 1;
            case TINYGLTF_COMPONENT_TYPE_SHORT:
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                return 2;
            case TINYGLTF_COMPONENT_TYPE_INT:
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                return 4;
            case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                return 8;
            default:
                throw std::runtime_error("Unsupported glTF component type");
        }
    }

    size_t component_count(const int gltf_type) {
        switch (gltf_type) {
            case TINYGLTF_TYPE_SCALAR:
                return 1;
            case TINYGLTF_TYPE_VEC2:
                return 2;
            case TINYGLTF_TYPE_VEC3:
                return 3;
            case TINYGLTF_TYPE_VEC4:
            case TINYGLTF_TYPE_MAT2:
                return 4;
            case TINYGLTF_TYPE_MAT3:
                return 9;
            case TINYGLTF_TYPE_MAT4:
                return 16;
            default:
                throw std::runtime_error("Unsupported glTF accessor type");
        }
    }

    size_t checked_required_bytes(const size_t element_size, const size_t count, const size_t byte_stride) {
        if (count == 0) {
            throw std::runtime_error("Accessor count must be > 0");
        }
        if (byte_stride == 0) {
            return element_size * count;
        }
        return byte_stride * (count - 1) + element_size;
    }

    void ensure_fits_int32(const size_t value, const char *what) {
        if (value > static_cast<size_t>(std::numeric_limits<int32>::max())) {
            throw std::runtime_error(std::string(what) + " exceeds int32 limit");
        }
    }
} // namespace

GltfHelper::GltfHelper() {
    m_model.asset.version = "2.0";
    m_model.asset.generator = "ApexPredator tinygltf helper";
    m_model.scenes.emplace_back();
    m_model.defaultScene = 0;
}

tinygltf::Model &GltfHelper::model() {
    return m_model;
}

const tinygltf::Model &GltfHelper::model() const {
    return m_model;
}

GltfHelper::Handle<tinygltf::Mesh> GltfHelper::create_mesh(const std::string &name, const int mode) {
    const auto mesh = make<tinygltf::Mesh>();
    mesh->name = name;
    mesh->primitives.emplace_back();
    mesh->primitives.back().mode = mode;
    return mesh;
}

int32 GltfHelper::create_primitive(const int32 mesh_id, const int mode) {
    if (mesh_id < 0 || static_cast<size_t>(mesh_id) >= m_model.meshes.size()) {
        throw std::runtime_error("Mesh id is out of range");
    }
    auto &mesh = m_model.meshes[static_cast<size_t>(mesh_id)];
    mesh.primitives.emplace_back();
    mesh.primitives.back().mode = mode;
    ensure_fits_int32(mesh.primitives.size() - 1, "Primitive index");
    return static_cast<int32>(mesh.primitives.size() - 1);
}

IndexedHandle<tinygltf::Accessor, int> GltfHelper::create_accessor_from_u8(
    const uint8 *data,
    const size_t byte_length,
    const int buffer_view_target,
    const int component_type,
    const int gltf_type,
    const size_t count,
    const bool normalized,
    const size_t byte_stride,
    const size_t accessor_byte_offset,
    const std::string &name
) {
    return create_accessor_chain_from_u8(
        data,
        byte_length,
        buffer_view_target,
        component_type,
        gltf_type,
        count,
        normalized,
        byte_stride,
        accessor_byte_offset,
        name
    ).accessor;
}

GltfHelper::AccessorChainIds GltfHelper::create_accessor_chain_from_u8(
    const uint8 *data,
    const size_t byte_length,
    const int buffer_view_target,
    const int component_type,
    const int gltf_type,
    const size_t count,
    const bool normalized,
    const size_t byte_stride,
    const size_t accessor_byte_offset,
    const std::string &name
) {
    if (data == nullptr && byte_length > 0) {
        throw std::runtime_error("Data pointer is null");
    }

    const size_t comp_size = component_size_bytes(component_type);
    const size_t elem_size = comp_size * component_count(gltf_type);

    if (byte_stride != 0 && byte_stride < elem_size) {
        throw std::runtime_error("Byte stride is smaller than element size");
    }
    if (accessor_byte_offset > byte_length) {
        throw std::runtime_error("Accessor byte offset is out of range");
    }

    const size_t available = byte_length - accessor_byte_offset;
    const size_t required = checked_required_bytes(elem_size, count, byte_stride);
    if (available < required) {
        throw std::runtime_error("Provided data is too small for accessor layout");
    }

    const auto buffer = make<tinygltf::Buffer>();
    buffer->data.resize(byte_length);
    if (byte_length > 0) {
        std::memcpy(buffer->data.data(), data, byte_length);
    }


    const auto buffer_view = make<tinygltf::BufferView>();
    buffer_view->buffer = buffer.index();
    buffer_view->byteOffset = 0;
    ensure_fits_int32(byte_length, "BufferView byteLength");
    buffer_view->byteLength = static_cast<int32>(byte_length);
    if (byte_stride > 0) {
        ensure_fits_int32(byte_stride, "BufferView byteStride");
        buffer_view->byteStride = static_cast<int32>(byte_stride);
    }
    buffer_view->target = buffer_view_target;

    const auto accessor = make<tinygltf::Accessor>();
    accessor->bufferView = buffer_view.index();
    ensure_fits_int32(accessor_byte_offset, "Accessor byteOffset");
    accessor->byteOffset = static_cast<int32>(accessor_byte_offset);
    accessor->componentType = component_type;
    accessor->type = gltf_type;
    ensure_fits_int32(count, "Accessor count");
    accessor->count = static_cast<int32>(count);
    accessor->normalized = normalized;
    accessor->name = name;


    return AccessorChainIds{
        .accessor = accessor,
        .buffer_view = buffer_view,
        .buffer = buffer
    };
}

IndexedHandle<tinygltf::Image, int32> GltfHelper::create_image_png_data(
    std::vector<uint8> &&png_data,
    const std::string &name
) {
    if (png_data.empty()) {
        throw std::runtime_error("PNG data size must be > 0");
    }

    const auto buffer = make<tinygltf::Buffer>();
    buffer->data = png_data;

    const auto buffer_view = make<tinygltf::BufferView>();
    buffer_view->buffer = buffer.index();
    buffer_view->byteOffset = 0;
    ensure_fits_int32(png_data.size(), "Image BufferView byteLength");
    buffer_view->byteLength = static_cast<int32>(png_data.size());
    buffer_view->target = 0;


    const auto image = make<tinygltf::Image>();
    image->name = name;
    image->mimeType = "image/png";
    image->bufferView = buffer_view.index();

    return image;
}

GltfHelper::Handle<tinygltf::Texture> GltfHelper::create_texture_png_data(
    std::vector<uint8> &&png_data,
    const std::string &name,
    Handle<tinygltf::Sampler> sampler
) {
    if (png_data.empty()) {
        throw std::runtime_error("PNG data size must be > 0");
    }
    const auto image = create_image_png_data(std::move(png_data), name);
    const auto texture = make<tinygltf::Texture>();
    texture->source = image.index();
    texture->name = name;
    if (sampler.is_valid()) {
        texture->sampler = sampler.index();
    }
    return texture;
}

GltfHelper::Handle<tinygltf::Accessor> GltfHelper::set_primitive_attribute_from_u8(
    const int32 mesh_id,
    const int32 primitive_index,
    const std::string &attribute_name,
    const uint8 *data,
    const size_t byte_length,
    const int component_type,
    const int gltf_type,
    const size_t count,
    const bool normalized,
    const size_t byte_stride,
    const size_t accessor_byte_offset,
    const std::string &accessor_name
) {
    if (mesh_id < 0 || static_cast<size_t>(mesh_id) >= m_model.meshes.size()) {
        throw std::runtime_error("Mesh id is out of range");
    }
    auto &mesh = m_model.meshes[static_cast<size_t>(mesh_id)];
    if (primitive_index < 0 || static_cast<size_t>(primitive_index) >= mesh.primitives.size()) {
        throw std::runtime_error("Primitive index is out of range");
    }

    const auto accessor = create_accessor_from_u8(
        data,
        byte_length,
        TINYGLTF_TARGET_ARRAY_BUFFER,
        component_type,
        gltf_type,
        count,
        normalized,
        byte_stride,
        accessor_byte_offset,
        accessor_name
    );

    mesh.primitives[static_cast<size_t>(primitive_index)].attributes[attribute_name] = accessor.index();
    return accessor;
}

GltfHelper::Handle<tinygltf::Accessor> GltfHelper::set_primitive_indices_from_u8(
    const int32 mesh_id,
    const int32 primitive_index,
    const uint8 *data,
    const size_t byte_length,
    const int component_type,
    const size_t count,
    const size_t byte_stride,
    const size_t accessor_byte_offset,
    const std::string &accessor_name
) {
    if (mesh_id < 0 || static_cast<size_t>(mesh_id) >= m_model.meshes.size()) {
        throw std::runtime_error("Mesh id is out of range");
    }
    auto &mesh = m_model.meshes[static_cast<size_t>(mesh_id)];
    if (primitive_index < 0 || static_cast<size_t>(primitive_index) >= mesh.primitives.size()) {
        throw std::runtime_error("Primitive index is out of range");
    }

    const auto accessor = create_accessor_from_u8(
        data,
        byte_length,
        TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER,
        component_type,
        TINYGLTF_TYPE_SCALAR,
        count,
        false,
        byte_stride,
        accessor_byte_offset,
        accessor_name
    );

    mesh.primitives[static_cast<size_t>(primitive_index)].indices = accessor.index();
    return accessor;
}

GltfHelper::Handle<tinygltf::Skin> GltfHelper::current_skin() const {
    if (m_skin_stack.empty()) {
        return {};
    }
    return m_skin_stack.back();
}

void GltfHelper::push_skin(const Handle<tinygltf::Skin> skin) {
    m_skin_stack.push_back(skin);
}

void GltfHelper::pop_skin() {
    m_skin_stack.pop_back();
}

void GltfHelper::reset() {
    m_model = tinygltf::Model();
}

bool is_all_zero(const glm::vec3 &v) {
    return glm::length(v) <= glm::epsilon<float>();
}

bool is_all_one(const glm::vec3 &v) {
    return glm::length(v - glm::vec3(1.0f)) <= glm::epsilon<float>();
}

bool is_identity_quat(const glm::quat &q) {
    constexpr auto eps = glm::epsilon<float>();
    constexpr auto id = glm::identity<glm::quat>();
    return glm::length(q - id) <= eps || glm::length(q + id) <= eps;
}

void GltfHelper::set_node_matrix(tinygltf::Node &node, const glm::mat4 &mat) {
    glm::vec3 translation, scale, skew;
    glm::vec4 perspective;
    glm::quat rotation;
    glm::decompose(mat, scale, rotation, translation, skew, perspective);

    if (glm::any(glm::isnan(translation))) {
        throw std::runtime_error("Translation is NaN");
    }
    if (glm::any(glm::isinf(translation))) {
        throw std::runtime_error("Translation is Inf");
    }
    if (glm::any(glm::isnan(scale))) {
        throw std::runtime_error("Scale is NaN");
    }
    if (glm::any(glm::isinf(scale))) {
        throw std::runtime_error("Scale is Inf");
    }
    if (glm::any(glm::isnan(rotation))) {
        throw std::runtime_error("Rotation is NaN");
    }
    if (glm::any(glm::isinf(rotation))) {
        throw std::runtime_error("Rotation is Inf");
    }

    if (!is_all_zero(translation)) {
        node.translation = {translation.x, translation.y, translation.z};
    }
    if (glm::abs(glm::length(rotation - glm::identity<glm::quat>())) > glm::epsilon<float>()) {
        node.rotation = {rotation.x, rotation.y, rotation.z, rotation.w};
    }
    if (!is_all_one(scale)) {
        node.scale = {scale.x, scale.y, scale.z};
    }
}

void GltfHelper::set_node_transform(tinygltf::Node &node, const glm::vec3 &translation, const glm::vec3 &scale,
                                    const glm::quat &rotation) {
    if (glm::any(glm::isnan(translation))) {
        throw std::runtime_error("Translation is NaN");
    }
    if (glm::any(glm::isinf(translation))) {
        throw std::runtime_error("Translation is Inf");
    }
    if (glm::any(glm::isnan(scale))) {
        throw std::runtime_error("Scale is NaN");
    }
    if (glm::any(glm::isinf(scale))) {
        throw std::runtime_error("Scale is Inf");
    }
    if (glm::any(glm::isnan(rotation))) {
        throw std::runtime_error("Rotation is NaN");
    }
    if (glm::any(glm::isinf(rotation))) {
        throw std::runtime_error("Rotation is Inf");
    }

    if (!is_all_zero(translation)) {
        node.translation = {translation.x, translation.y, translation.z};
    }
    if (!is_identity_quat(rotation)) {
        node.rotation = {rotation.x, rotation.y, rotation.z, rotation.w};
    }
    if (!is_all_one(scale)) {
        node.scale = {scale.x, scale.y, scale.z};
    }
}

void GltfHelper::add_extra_save_data(const std::string &name, const std::vector<uint8> &&data) {
    m_extra_save_data.emplace_back(name, data);
}

void GltfHelper::add_to_scene(const Handle<tinygltf::Node> node) {
    if (node.index() < 0 || static_cast<size_t>(node.index()) >= m_model.nodes.size()) {
        throw std::runtime_error("Node id is out of range");
    }
    if (m_model.scenes.empty()) {
        m_model.scenes.emplace_back();
    }
    m_model.scenes[0].nodes.push_back(node.index());
}

GltfHelper::Handle<tinygltf::Node> GltfHelper::get_parent(const Handle<tinygltf::Node> &child) {
    const auto child_index = child.index();
    for (const auto [index, value]: m_model.nodes | std::views::enumerate) {
        if (std::ranges::contains(value.children, child_index)) {
            return {&m_model.nodes, static_cast<int32>(index)};
        }
    }
    return {};
}

void GltfHelper::set_parent(const Handle<tinygltf::Node> &parent, const Handle<tinygltf::Node> &children) {
    if (children.index() < 0 || static_cast<size_t>(children.index()) >= m_model.nodes.size()) {
        throw std::runtime_error("Node id is out of range");
    }
    if (parent.index() < 0 || static_cast<size_t>(parent.index()) >= m_model.nodes.size()) {
        throw std::runtime_error("Parent id is out of range");
    }

    if (children == parent) {
        GLog_Error("Cannot set node to itself as parent");
        return;
    }

    if (std::ranges::contains(m_model.nodes[parent.index()].children, children.index())) {
        GLog_Warning("Node() is already a child of parent()", children.index(), parent.index());
        return;
    }

    m_model.nodes[parent.index()].children.push_back(children.index());
}

GltfHelper::Handle<tinygltf::Node> GltfHelper::find_node_in_skin(Handle<tinygltf::Skin> skin,
                                                                 std::string_view name) {
    for (const auto node_id: skin->joints) {
        const auto &node = model().nodes[node_id];
        if (node.name == name) {
            return {&model().nodes, node_id};
        }
    }
    return {};
}
