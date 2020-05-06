

#pragma once

#include <unordered_map>

#include <gl_handlers.hpp>
#include <scene/material.hpp>

namespace scene
{
  class mesh
  {
  public:
    mesh(int32_t vsources_idx, int32_t isource_idx, uint32_t, uint32_t, uint32_t);
    mesh(mesh&&) = default;
    mesh& operator=(mesh&&) = default;
    ~mesh() = default;
    int32_t get_vertices() const;
    int32_t get_indices() const;
    uint32_t get_indices_type() const;
    uint32_t get_indices_size() const;
    uint32_t get_vertices_size() const;
  private:
    int32_t m_vertices;
    int32_t m_indices;
    GLenum m_indices_type;
    uint32_t m_indices_size;
    uint32_t m_vertices_size;
  //    std::vector<gl::texture<GL_TEXTURE_2D>> animations;
  };
}
