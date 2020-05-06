

#pragma once

#include <unordered_map>

#include <gl_handlers.hpp>

namespace scene
{
  using texture = std::variant<gl::texture<GL_TEXTURE_2D>, gl::texture<GL_TEXTURE_CUBE_MAP>, gl::texture<GL_TEXTURE_1D>, gl::texture<GL_TEXTURE_3D>>;

  class material
  {
  public:
    explicit material(uint32_t program_idx);
    material(material&&) = default;
    material& operator=(material&&) = default;
    ~material() = default;
    void set_state(gl::gpu_state);
    const gl::gpu_state& get_state() const;
    void add_texture(const std::string&, uint32_t);
    uint32_t get_program() const;
    const std::unordered_map<std::string, uint32_t>& get_textures() const;
  private:
    uint32_t m_program;
    std::unordered_map<std::string, uint32_t> m_textures;
    gl::gpu_state m_state {};
  };
}
