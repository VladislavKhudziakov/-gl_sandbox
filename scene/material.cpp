

#include "material.hpp"


scene::material::material(uint32_t shader_idx)
: m_program(shader_idx)
{
}


void scene::material::set_state(gl::gpu_state state)
{
  m_state = state;
}


const gl::gpu_state &scene::material::get_state() const
{
  return m_state;
}


void scene::material::add_texture(const std::string& sampler, uint32_t i)
{
  m_textures.emplace(sampler, i);
}


uint32_t scene::material::get_program() const
{
  return m_program;
}


const std::unordered_map<std::string, uint32_t>& scene::material::get_textures() const
{
  return m_textures;
}
