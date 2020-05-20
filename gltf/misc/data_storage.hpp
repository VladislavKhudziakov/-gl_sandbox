


#pragma once

#include <vector>
#include <cinttypes>

namespace gltf
{
    struct data_storage
    {
        enum class component_type
        {
            i8 = 5120, u8, i16, u16, i32, u32, f32, f64
        };

        enum class type
        {
            vec2 = 2, vec3, vec4, scalar = 64 + 1, vector = 64 + 4, matrix = 64 + 16
        };

        std::vector<uint8_t> data;
        data_storage::component_type c_type;
        type d_type;
        bool normalized;
    };
}




