


#pragma once

#include <gltf/misc/data_storage.hpp>

#include <third/tinygltf/tiny_gltf.h>

#include <tuple>


namespace gltf::utils
{
    using accessor_data = std::tuple<const tinygltf::Accessor&, const tinygltf::BufferView&, const tinygltf::Buffer&, const uint8_t*, size_t>;


    inline accessor_data get_buffer_data(const tinygltf::Model &mdl, uint32_t accessor_idx)
    {
        return {
            mdl.accessors.at(accessor_idx),
            mdl.bufferViews.at(mdl.accessors.at(accessor_idx).bufferView),
            mdl.buffers.at(mdl.bufferViews.at(mdl.accessors.at(accessor_idx).bufferView).buffer),
            mdl.buffers.at(mdl.bufferViews.at(mdl.accessors.at(accessor_idx).bufferView).buffer).data.data() +
            mdl.bufferViews.at(mdl.accessors.at(accessor_idx).bufferView).byteOffset,
            mdl.bufferViews.at(mdl.accessors.at(accessor_idx).bufferView).byteLength
        };
    }


    template <typename Callable, typename Container>
    void copy_buffer_data(Callable&& get_data_ptr, Container& container, const tinygltf::Model& mdl, uint32_t accessor_idx)
    {
        auto [data_accessor, data_b_view, data_buf, data_ptr, data_size] = get_buffer_data(mdl, accessor_idx);
        container.reserve(container.size() + data_accessor.count);
        const auto bytes_size = tinygltf::GetComponentSizeInBytes(data_accessor.componentType);
        const auto components_count = tinygltf::GetNumComponentsInType(data_accessor.type);

        for (size_t i = 0; i < data_accessor.count; ++i) {
            auto& curr_el = container.emplace_back();
            assert(sizeof(curr_el) == bytes_size * components_count);
            std::memcpy(get_data_ptr(container.back()), data_ptr + data_accessor.byteOffset, sizeof(curr_el));
            data_ptr += data_b_view.byteStride;
        }
    }


    inline void copy_buffer_bytes(data_storage& ds, const tinygltf::Model& mdl, uint32_t accessor_idx)
    {
        auto [data_accessor, data_b_view, data_buf, data_ptr, data_size] = get_buffer_data(mdl, accessor_idx);
        ds.c_type = static_cast<data_storage::component_type>(data_accessor.componentType);
        ds.d_type = static_cast<data_storage::type>(data_accessor.type);

        const auto type_size = tinygltf::GetComponentSizeInBytes(data_accessor.componentType);
        const auto type_components = tinygltf::GetNumComponentsInType(data_accessor.type);
        const auto element_size = type_size * type_components;

        ds.data.resize(data_accessor.count * element_size);

        auto dst_data_ptr = ds.data.data();

        for (size_t i = 0; i < data_accessor.count; ++i) {
            std::memcpy(dst_data_ptr, data_ptr + data_accessor.byteOffset, element_size);
            data_ptr += data_b_view.byteStride;
            dst_data_ptr += element_size;
        }
    }
}



