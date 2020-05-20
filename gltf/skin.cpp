

#include "skin.hpp"

#include <gltf/misc/acessor_utils.hpp>

#include <glm/gtc/type_ptr.hpp>

gltf::skin::skin(const tinygltf::Model& model, const tinygltf::Skin& skin, const scene_graph& graph)
{
    for (const auto joint : skin.joints) {
        std::shared_ptr<scene_graph::node> node;

        graph.go_though([this, &node, joint](const std::shared_ptr<scene_graph::node>& graph_node) {
            if (joint == graph_node->get_node_index()) {
                node = graph_node;
                return false;
            }
            return true;
        });

        m_nodes.emplace_back(node);
    }

    utils::copy_buffer_data(
        [](glm::mat4& data) { return glm::value_ptr(data); },
        m_inv_bind_poses,
        model,
        skin.inverseBindMatrices);
}


const std::string& gltf::skin::get_name() const
{
    return m_name;
}


const std::vector<glm::mat4>& gltf::skin::get_nodes_matrices() const
{
    return m_inv_bind_poses;
}


const std::vector<std::shared_ptr<gltf::scene_graph::node>>& gltf::skin::get_nodes() const
{
    return m_nodes;
}
