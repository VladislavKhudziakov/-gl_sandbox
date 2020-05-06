

#pragma once

#include <gl/vertex_array_object.hpp>
#include <gl/buffer.hpp>
#include <gl/textures.hpp>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <vector>
#include <optional>

namespace loader
{
struct geom_subset
{
  uint64_t base_vertex;
  uint64_t length;
};

  struct mesh_instance
  {
    gl::vertex_array_object vertices;
    gl::buffer<GL_ELEMENT_ARRAY_BUFFER> indices;
    std::vector<geom_subset> subsets;
  };

  struct vertex
  {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 uv;
  };

  mesh_instance load_model(const std::string& file_name);
  gl::texture<GL_TEXTURE_2D> load_tex_2d(const std::string&);
  gl::texture<GL_TEXTURE_CUBE_MAP> load_tex_cube(const std::string&, uint32_t w = 512, uint32_t h = 512);
  gl::texture<GL_TEXTURE_CUBE_MAP> load_diff_ibl(const gl::texture<GL_TEXTURE_CUBE_MAP>& tex);
  std::pair<gl::texture<GL_TEXTURE_CUBE_MAP>, gl::texture<GL_TEXTURE_2D>> load_spec_ibl(const gl::texture<GL_TEXTURE_CUBE_MAP>& tex);
}

