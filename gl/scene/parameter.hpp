

#pragma once

#include <cinttypes>
#include <cstring>
#include <vector>

namespace gl::scene
{
    enum class parameter_type
    {
        f32,
        i32
    };


    enum class parameter_component_type
    {
        vec1,
        vec2,
        vec3,
        vec4,
        mat2,
        mat3,
        mat4
    };


    inline constexpr auto get_param_components_size(parameter_component_type param_type)
    {
        constexpr uint32_t params_sizes[]{
            1, 2, 3, 4, 2 * 2, 3 * 3, 4 * 4};

        return params_sizes[uint32_t(param_type)];
    }


    inline constexpr auto get_param_type_size(parameter_type param_type)
    {
        constexpr uint32_t params_sizes[]{
            sizeof(float), sizeof(int32_t)};

        return params_sizes[uint32_t(param_type)];
    }


    class parameter
    {
    public:
        parameter(parameter_type, parameter_component_type);
        parameter(parameter_type, parameter_component_type, void* data);
        ~parameter() = default;

        uint8_t* get_data();
        const uint8_t* get_data() const;
        parameter_type get_param_type() const;
        parameter_component_type get_component_type() const;

    private:
        std::vector<uint8_t> m_data;
        parameter_type m_type;
        parameter_component_type m_component_type;
    };
} // namespace gl::scene
