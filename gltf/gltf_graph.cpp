

#include "gltf_graph.hpp"

#include <third/tinygltf/tiny_gltf.h>


gltf::scene_graph::scene_graph(const tinygltf::Model& mdl, uint32_t scene_index)
    : m_model(mdl)
    , m_scene_index(scene_index)
{
    make_scene_graph();
}


gltf::scene_graph::~scene_graph() = default;


void gltf::scene_graph::update()
{
    for_each_node(m_root, [](std::shared_ptr<node>& node) {
        auto parent_ptr = node->m_parent.lock();
        if (parent_ptr != nullptr) {
            node->m_world_matrix = parent_ptr->m_world_matrix * node->get_local_transformation();
        } else {
            node->m_world_matrix = node->get_local_transformation();
        }

        return true;
    });
}


std::shared_ptr<gltf::scene_graph::node> gltf::scene_graph::get_root() const
{
    return std::shared_ptr<node>();
}

void gltf::scene_graph::make_scene_graph()
{
    const auto& scene = m_model.scenes.at(m_scene_index);
    m_root = std::make_shared<scene_graph::node>();

    for (const auto scene_node_index : scene.nodes) {
        make_scene_node(scene_node_index, m_root);
    }
}


void gltf::scene_graph::make_scene_node(uint32_t node_index, std::shared_ptr<scene_graph::node>& parent)
{
    auto curr_node = std::make_shared<scene_graph::node>();
    curr_node->m_node_index = node_index;
    const auto& model_node = m_model.nodes.at(node_index);

    if (!model_node.translation.empty()) {
        curr_node->translation = glm::vec3{model_node.translation[0], model_node.translation[1], model_node.translation[2]};
    }

    if (!model_node.scale.empty()) {
        curr_node->scale = glm::vec3{model_node.scale[0], model_node.scale[1], model_node.scale[2]};
    }

    if (!model_node.rotation.empty()) {
        curr_node->rotation = glm::quat{
            static_cast<float>(model_node.rotation[3]),
            static_cast<float>(model_node.rotation[0]),
            static_cast<float>(model_node.rotation[1]),
            static_cast<float>(model_node.rotation[2])};
    }

    curr_node->m_parent = parent;
    parent->m_children.emplace_back(curr_node);

    for (const auto scene_node_index : model_node.children) {
        make_scene_node(scene_node_index, curr_node);
    }
}


void gltf::scene_graph::go_though(
    std::function<bool(const std::shared_ptr<node>&)> f) const
{
    for_each_node(m_root, f);
}


void gltf::scene_graph::go_though(
    std::function<bool(std::shared_ptr<node>&)> f)
{
    for_each_node(m_root, f);
}


void gltf::scene_graph::for_each_node(
    std::shared_ptr<scene_graph::node>& graph_node,
    const std::function<bool(std::shared_ptr<scene_graph::node>&)>& f)
{
    if (!f(graph_node)) {
        return;
    }

    for (auto& child : graph_node->m_children) {
        for_each_node(child, f);
    }
}


void gltf::scene_graph::for_each_node(
    const std::shared_ptr<scene_graph::node>& graph_node,
    const std::function<bool(const std::shared_ptr<scene_graph::node>&)>& f) const
{
    if (!f(graph_node)) {
        return;
    }

    for (auto& child : graph_node->m_children) {
        for_each_node(child, f);
    }
}


glm::mat4 gltf::scene_graph::node::get_local_transformation() const
{
    auto t = glm::translate(glm::mat4{1}, translation);
    auto r = glm::mat4_cast(rotation);
    auto s = glm::scale(glm::mat4{1}, scale);
    return t * r * s;
}


glm::mat4 gltf::scene_graph::node::get_global_transformation() const
{
    return m_world_matrix;
}


uint32_t gltf::scene_graph::node::get_node_index() const
{
    return m_node_index;
}
