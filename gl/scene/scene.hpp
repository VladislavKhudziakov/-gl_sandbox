

#pragma once

#include <gl/scene/meshes.hpp>
#include <gl/framebuffer_object.hpp>
#include <gl/scene/attachment.hpp>
#include <gl/scene/framebuffer.hpp>
#include <gl/scene/pass.hpp>

#include <vector>

namespace gl::scene
{

  struct camera
  {
    float fov;
    glm::vec3 forward;
    glm::vec3 position;
    glm::vec3 up;
    float near;
    float far;
  };


  struct transformation
  {
    glm::vec3 translation {0, 0, 0};
    glm::vec3 rotation {0, 0, 0};
    glm::vec3 scale {1, 1, 1};
  };


  struct scene_node
  {
    uint32_t mesh_idx = -1;
    uint32_t material_idx = -1;
    std::vector<uint32_t> children;
    transformation transformation;
  };


  struct scene
  {
    camera camera;

    std::vector<gl::scene::mesh> meshes;
    std::vector<gl::scene::material> materials;
    std::vector<gl::scene::scene_node> nodes;
    std::vector<gl::scene::attachment> attachments;
    std::vector<gl::scene::framebuffer> framebuffers;
    std::vector<gl::scene::pass> passes;
    std::vector<gl::scene::texture> textures;

    std::vector<gl::framebuffer_object> fbos;
    std::vector<gl::program> shaders;
    std::vector<gl::vertex_array_object> vertex_sources;
    std::vector<gl::buffer<GL_ELEMENT_ARRAY_BUFFER>> index_sources;
  };

  void draw(const scene& s, const std::vector<uint32_t>&, uint32_t pass_idx);
}

