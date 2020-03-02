

#include <gltf_handlers.hpp>

#include <glm/gtc/matrix_access.hpp>

std::shared_ptr<gltf::scene_node>gltf::make_graph(
    const tinygltf::Model &mdl,
    const tinygltf::Node& node,
   const std::shared_ptr<scene_node>& parent,
    std::vector<mesh>& meshes)
{
  auto curr_node = std::make_shared<gltf::scene_node>();
  curr_node->m_parent = parent;

  if (!node.translation.empty()) {
    curr_node->m_translation = {node.translation[0], node.translation[1], node.translation[2]};
  }

  if (!node.rotation.empty()) {
    curr_node->m_rotation = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
  }

  if (!node.scale.empty()) {
    curr_node->m_scale = {node.scale[0], node.scale[1], node.scale[2]};
  }

  curr_node->m_world_matrix = parent->m_world_matrix * curr_node->calc_TRS();

  if (node.mesh >= 0) {
    for (const auto& primitive : mdl.meshes.at(node.mesh).primitives) {
      meshes.emplace_back();

      auto [pos_accessor, pos_b_view, pos_buf, pos_data, pos_size] = gltf::get_buffer_data(mdl, primitive.attributes.at("POSITION"));
      gl::buffer<GL_ARRAY_BUFFER> positions_buffer;
      positions_buffer.fill(const_cast<uint8_t*>(pos_data), pos_size);
      meshes.back().vao.add_vertex_array<float>(positions_buffer, 3, pos_accessor.ByteStride(pos_b_view));

      auto [norm_accessor, norm_b_view, norm_buf, norm_data, norm_size] = gltf::get_buffer_data(mdl, primitive.attributes.at("NORMAL"));
      gl::buffer<GL_ARRAY_BUFFER> normals_buffer;
      normals_buffer.fill(const_cast<uint8_t*>(norm_data), norm_size);
      meshes.back().vao.add_vertex_array<float>(normals_buffer, 3, norm_accessor.ByteStride(norm_b_view));

      auto [joints_accessor, joints_b_view, joints_buf, joints_data, joints_size] = gltf::get_buffer_data(mdl, primitive.attributes.at("JOINTS_0"));
      gl::buffer<GL_ARRAY_BUFFER> joints_buffer;
        joints_buffer.fill(const_cast<uint8_t*>(joints_data), joints_size);

      meshes.back().vao.add_vertex_array<uint16_t>(
              joints_buffer,
              tinygltf::GetNumComponentsInType(joints_accessor.type),
              joints_accessor.ByteStride(joints_b_view),
              0, joints_accessor.componentType,
              joints_accessor.normalized);

      auto [weights_accessor, weights_b_view, weights_buf, weights_data, weights_size] = gltf::get_buffer_data(mdl, primitive.attributes.at("WEIGHTS_0"));
      gl::buffer<GL_ARRAY_BUFFER> weights_buffer;
      weights_buffer.fill(const_cast<uint8_t*>(weights_data), weights_size);

      meshes.back().vao.add_vertex_array<float>(
              weights_buffer,
              tinygltf::GetNumComponentsInType(joints_accessor.type),
              weights_accessor.ByteStride(weights_b_view),
              0, weights_accessor.componentType,
              weights_accessor.normalized);

      auto [ind_accessor, ind_b_view, ind_buf, ind_data, ind_size] = gltf::get_buffer_data(mdl, primitive.indices);
      meshes.back().indices.fill(const_cast<uint8_t*>(ind_data), ind_size);
      meshes.back().indices_count = ind_accessor.count;
      meshes.back().world_transform = curr_node->m_world_matrix;
      meshes.back().skin_idx = node.skin;
    }
  }

  for (auto node_idx : node.children) {
    curr_node->m_children.emplace_back(gltf::make_graph(mdl, mdl.nodes.at(node_idx), curr_node, meshes));
    curr_node->m_children.back()->m_gltf_idx = node_idx;
  }

  return curr_node;
}

const glm::mat4 &gltf::scene_node::get_world_matrix() const
{
  return m_world_matrix;
}

glm::mat4 gltf::scene_node::calc_TRS() const
{
  auto t = glm::translate(glm::mat4{1}, m_translation);
  auto r = glm::mat4_cast(m_rotation);
  auto s = glm::scale(glm::mat4{1}, m_scale);
  return t * r * s;
}

const glm::mat4 &gltf::scene_node::get_inv_bind_matrix() const
{
  return m_inv_bind_matrix;
}

std::shared_ptr<gltf::scene_node> gltf::make_graph(const tinygltf::Model &mdl, std::vector<mesh>& meshes)
{
  auto root = std::make_shared<gltf::scene_node>();
  root->m_gltf_idx = -1;

  for (auto node_idx : mdl.scenes.at(0).nodes) {
    root->m_children.emplace_back(gltf::make_graph(mdl, mdl.nodes.at(node_idx), root, meshes));
    root->m_children.back()->m_gltf_idx = node_idx;
  }

  return root;
}


std::shared_ptr<gltf::scene_node> gltf::find_node_by_index(
    const std::shared_ptr<scene_node> & n, uint32_t idx) {
    if (n->m_gltf_idx == idx) {
      return n;
    }

    for (const auto& child : n->m_children) {
      if (const auto founded_child = find_node_by_index(child, idx); founded_child != nullptr) {
        return founded_child;
      }
    }

    return nullptr;
}
std::vector<gltf::skin> gltf::get_skins(
    const tinygltf::Model &mdl,
    std::shared_ptr<scene_node> root)
{
  std::vector<skin> skins;
  skins.reserve(mdl.skins.size());

  for (const auto& skin : mdl.skins) {
    skins.emplace_back();
    skins.back().world_matrices.reserve(skin.joints.size());
    skins.back().inv_bind_poses.resize(skin.joints.size());

    auto [ib_accessor, ib_b_view, ib_buf, ib_data, ib_data_size] = get_buffer_data(mdl, skin.inverseBindMatrices);
    std::memcpy(skins.back().inv_bind_poses.data(), ib_data, ib_data_size);

    for (auto joint : skin.joints) {
      skins.back().world_matrices.emplace_back(find_node_by_index(root, joint)->get_world_matrix());
    }
  }
  return skins;
}
