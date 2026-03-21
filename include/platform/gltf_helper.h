// Created by RED on 08.03.2026.
#pragma once
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include "int_def.h"
#include "tiny_gltf.h"
#include "platform/logger.h"

#include "glm/glm.hpp"

struct DataBlob {
    std::string name;
    std::vector<uint8> data;
};

template<typename T, typename IndexT = size_t>
class IndexedHandle {
public:
    using element_type = T;

    IndexedHandle() = default;

    IndexedHandle(std::vector<T> *storage, const IndexT index)
        : m_storage(storage), m_index(index) {
    }

    [[nodiscard]] T *get() const {
        if (!m_storage || m_index >= m_storage->size()) {
            return nullptr;
        }
        return std::addressof((*m_storage)[m_index]);
    }

    T &operator*() const {
        T *ptr = get();
        assert(ptr);
        return *ptr;
    }

    T *operator->() const {
        return get();
    }

    explicit operator bool() const {
        return get() != nullptr;
    }

    [[nodiscard]] IndexT index() const {
        return m_index;
    }

    void reset() {
        m_storage = nullptr;
        m_index = invalid_index();
    }

    void reset(std::vector<T> *storage, const IndexT index) {
        m_storage = storage;
        m_index = index;
    }

    T *release() {
        T *ptr = get();
        reset();
        return ptr;
    }

    [[nodiscard]] bool is_valid() const {
        return m_storage != nullptr && m_index != invalid_index();
    }

    friend bool operator==(const IndexedHandle &a, const IndexedHandle &b) {
        return a.m_storage == b.m_storage && a.m_index == b.m_index;
    }

    friend bool operator!=(const IndexedHandle &a, const IndexedHandle &b) {
        return !(a == b);
    }

private:
    static constexpr IndexT invalid_index() {
        return static_cast<IndexT>(-1);
    }

    std::vector<T> *m_storage = nullptr;
    IndexT m_index = invalid_index();
};

class GltfHelper {
public:
    template<typename T>
    using Handle = IndexedHandle<T, int32>;

    struct AccessorChainIds {
        Handle<tinygltf::Accessor> accessor;
        Handle<tinygltf::BufferView> buffer_view;
        Handle<tinygltf::Buffer> buffer;
    };

    GltfHelper();

    template<typename T>
    Handle<T> make();

    template<typename T>
    Handle<T> get(int32 index);

    template<typename T>
    Handle<T> find(std::string_view name);

    [[nodiscard]] tinygltf::Model &model();

    [[nodiscard]] const tinygltf::Model &model() const;

    Handle<tinygltf::Mesh> create_mesh(const std::string &name = {}, int mode = TINYGLTF_MODE_TRIANGLES);

    int32 create_primitive(int32 mesh_id, int mode = TINYGLTF_MODE_TRIANGLES);

    Handle<tinygltf::Accessor> create_accessor_from_u8(
        const uint8 *data,
        size_t byte_length,
        int buffer_view_target,
        int component_type,
        int gltf_type,
        size_t count,
        bool normalized = false,
        size_t byte_stride = 0,
        size_t accessor_byte_offset = 0,
        const std::string &name = {}
    );

    AccessorChainIds create_accessor_chain_from_u8(
        const uint8 *data,
        size_t byte_length,
        int buffer_view_target,
        int component_type,
        int gltf_type,
        size_t count,
        bool normalized = false,
        size_t byte_stride = 0,
        size_t accessor_byte_offset = 0,
        const std::string &name = {}
    );

    Handle<tinygltf::Image> create_image_png_data(
        std::vector<uint8> &&png_data,
        const std::string &name = {}
    );


    Handle<tinygltf::Texture> create_texture_png_data(
        std::vector<uint8> &&png_data,
        const std::string &name = {},
        Handle<tinygltf::Sampler> sampler = {}
    );


    Handle<tinygltf::Accessor> set_primitive_attribute_from_u8(
        int32 mesh_id,
        int32 primitive_index,
        const std::string &attribute_name,
        const uint8 *data,
        size_t byte_length,
        int component_type,
        int gltf_type,
        size_t count,
        bool normalized = false,
        size_t byte_stride = 0,
        size_t accessor_byte_offset = 0,
        const std::string &accessor_name = {}
    );

    Handle<tinygltf::Accessor> set_primitive_indices_from_u8(
        int32 mesh_id,
        int32 primitive_index,
        const uint8 *data,
        size_t byte_length,
        int component_type,
        size_t count,
        size_t byte_stride = 0,
        size_t accessor_byte_offset = 0,
        const std::string &accessor_name = {}
    );

    [[nodiscard]] Handle<tinygltf::Skin> current_skin() const;

    void push_skin(Handle<tinygltf::Skin> skin);

    void pop_skin();

    void reset();

    static void set_node_matrix(tinygltf::Node &node, const glm::mat4 &mat);

    static void set_node_transform(tinygltf::Node &node,
                                   const glm::vec3 &translation,
                                   const glm::vec3 &scale,
                                   const glm::quat &rotation);

    void add_extra_save_data(const std::string &name, const std::vector<uint8> &&data);

    void add_to_scene(Handle<tinygltf::Node> node);

    [[nodiscard]] Handle<tinygltf::Node> get_parent(const Handle<tinygltf::Node> &child);

    void set_parent(const Handle<tinygltf::Node> &parent, const Handle<tinygltf::Node> &children);

    Handle<tinygltf::Node> find_node_in_skin(Handle<tinygltf::Skin> skin, std::string_view name);

private:
    tinygltf::Model m_model{};
    std::vector<Handle<tinygltf::Skin> > m_skin_stack;
    std::vector<DataBlob> m_extra_save_data;
};

#define MAKE_FUNCTIONS(TYPE, STORAGE)\
    template<>\
    inline GltfHelper::Handle<TYPE> GltfHelper::make<TYPE>() {\
        if (m_model.STORAGE.size() > std::numeric_limits<int32>::max()) {\
            throw std::runtime_error(#TYPE " count exceeds int32 limit");\
        }\
        auto &obj = m_model.STORAGE.emplace_back();\
        auto index = (m_model.STORAGE.size() - 1);\
        return {&m_model.STORAGE, (int32)index};\
    }\
    template<>\
    inline GltfHelper::Handle<TYPE> GltfHelper::find<TYPE>(const std::string_view name) {\
        for (size_t i = 0; i < m_model.STORAGE.size(); i++) {\
            if (m_model.STORAGE[i].name == name) {\
                return {&m_model.STORAGE, (int32)i};\
            }\
        }\
        return {};\
    }\
    template<>\
    inline GltfHelper::Handle<TYPE> GltfHelper::get(int32 index){\
        if(index<0 || index >= m_model.STORAGE.size()){\
            GLog_Warning("Index out of range");\
            return {};\
        }\
        return {&m_model.STORAGE, index};\
    }

MAKE_FUNCTIONS(tinygltf::Node, nodes)
MAKE_FUNCTIONS(tinygltf::Mesh, meshes)
MAKE_FUNCTIONS(tinygltf::Accessor, accessors)
MAKE_FUNCTIONS(tinygltf::Buffer, buffers)
MAKE_FUNCTIONS(tinygltf::BufferView, bufferViews)
MAKE_FUNCTIONS(tinygltf::Material, materials)
MAKE_FUNCTIONS(tinygltf::Texture, textures)
MAKE_FUNCTIONS(tinygltf::Image, images)
MAKE_FUNCTIONS(tinygltf::Animation, animations)
MAKE_FUNCTIONS(tinygltf::Sampler, samplers)
MAKE_FUNCTIONS(tinygltf::Skin, skins)

#undef MAKE_FUNCTIONS
