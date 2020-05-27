


#pragma once

#include <gltf/misc/data_storage.hpp>

namespace gltf::utils
{
    inline uint32_t get_element_size(gltf::data_storage::component_type t)
    {
        switch (t) {
            case gltf::data_storage::component_type::i8:
                [[fallthrough]];
            case gltf::data_storage::component_type::u8:
                return 1;
            case gltf::data_storage::component_type::i16:
                [[fallthrough]];
            case gltf::data_storage::component_type::u16:
                return 2;
            case gltf::data_storage::component_type::i32:
                [[fallthrough]];
            case gltf::data_storage::component_type::u32:
                [[fallthrough]];
            case gltf::data_storage::component_type::f32:
                return 4;
            case gltf::data_storage::component_type::f64:
                return 8;
        }
    }

    inline uint32_t get_elements_count(gltf::data_storage::type t)
    {
        switch (t) {
            case gltf::data_storage::type::vec2:
                return 2;
            case gltf::data_storage::type::vec3:
                return 3;
            case gltf::data_storage::type::vec4:
                return 4;
            case gltf::data_storage::type::scalar:
                return 1;
            case gltf::data_storage::type::matrix:
                return 4 * 4;
            default:
                throw std::runtime_error("unsupported type");
        }
    }
}
