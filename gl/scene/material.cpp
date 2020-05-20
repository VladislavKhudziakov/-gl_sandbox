

#include "material.hpp"


gl::scene::material::material(uint32_t shader_idx)
    : m_program(shader_idx)
{
}


void gl::scene::material::set_state(gl::scene::gpu_state state)
{
    m_state = state;
}


const gl::scene::gpu_state& gl::scene::material::get_state() const
{
    return m_state;
}


void gl::scene::material::add_texture(const std::string& sampler, uint32_t i)
{
    m_textures.emplace(sampler, i);
}


uint32_t gl::scene::material::get_program() const
{
    return m_program;
}


const std::unordered_map<std::string, uint32_t>& gl::scene::material::get_textures() const
{
    return m_textures;
}


void gl::scene::material::add_parameter(std::string param_name, uint32_t param_idx)
{
    m_parameters.emplace(param_name, param_idx);
}


const std::unordered_map<std::string, uint32_t>& gl::scene::material::get_parameters() const
{
    return m_parameters;
}
