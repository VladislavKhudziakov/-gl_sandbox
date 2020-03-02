


#pragma once
#include <memory>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <gl_handlers.hpp>
#include <third/tinygltf/tiny_gltf.h>

namespace gltf
{
  struct mesh
  {
    gl::vertex_array_object vao;
    gl::buffer<GL_ELEMENT_ARRAY_BUFFER> indices;
    size_t indices_count;
    glm::mat4 world_transform;
    uint32_t skin_idx;
  };

  struct skin
  {
    std::vector<glm::mat4> world_matrices;
    std::vector<glm::mat4> inv_bind_poses;
  };

  class scene_node
  {
  public:
    const glm::mat4& get_world_matrix() const;
    const glm::mat4& get_inv_bind_matrix() const;
  private:
    glm::mat4 calc_TRS() const;

    uint32_t m_gltf_idx;
    glm::mat4 m_world_matrix {1};
    glm::mat4 m_inv_bind_matrix {1};
    glm::vec3 m_translation {0};
    glm::vec3 m_scale {1};
    glm::quat m_rotation {1.0, 0.0, 0.0, 0.0};

    std::weak_ptr<scene_node> m_parent {};
    std::vector<std::shared_ptr<scene_node>> m_children {};
    friend std::shared_ptr<scene_node> make_graph(
        const tinygltf::Model& mdl,
        const tinygltf::Node& node,
        const std::shared_ptr<scene_node>& parent,
        std::vector<mesh>& meshes);
    friend std::shared_ptr<scene_node> make_graph(const tinygltf::Model& mdl, std::vector<mesh>& meshes);
    friend std::shared_ptr<scene_node> find_node_by_index(const std::shared_ptr<scene_node>&, uint32_t);
  };

  std::shared_ptr<scene_node> make_graph(const tinygltf::Model& mdl, std::vector<mesh>& meshes);
  std::shared_ptr<scene_node> find_node_by_index(const std::shared_ptr<scene_node>&, uint32_t);

using accessor_data = std::tuple<
    const tinygltf::Accessor&,
    const tinygltf::BufferView&,
    const tinygltf::Buffer&,
    const uint8_t*,
    size_t>;

  inline accessor_data get_buffer_data(const tinygltf::Model &mdl, uint32_t accessor_idx)
  {
    return accessor_data {
        mdl.accessors.at(accessor_idx),
        mdl.bufferViews.at(mdl.accessors.at(accessor_idx).bufferView),
        mdl.buffers.at(mdl.bufferViews.at(mdl.accessors.at(accessor_idx).bufferView).buffer),
        mdl.buffers.at(mdl.bufferViews.at(mdl.accessors.at(accessor_idx).bufferView).buffer).data.data() +
        mdl.bufferViews.at(mdl.accessors.at(accessor_idx).bufferView).byteOffset
        + mdl.accessors.at(accessor_idx).byteOffset,
        mdl.bufferViews.at(mdl.accessors.at(accessor_idx).bufferView).byteLength - mdl.accessors.at(accessor_idx).byteOffset
    };
  }

  std::vector<skin> get_skins(const tinygltf::Model &mdl, std::shared_ptr<scene_node> root);
}

