

#include "for_each_node.hpp"

#include <third/tinygltf/tiny_gltf.h>

namespace
{
    void for_each_node(const tinygltf::Model& mdl, uint32_t node_index, const std::function<void(const tinygltf::Node)>& f)
    {
        const auto& node = mdl.nodes.at(node_index);
        f(node);
        for (const auto child_node_index : node.children) {
            for_each_node(mdl, child_node_index, f);
        }
    }
}


void gltf::utils::for_each_node(
        const tinygltf::Model& mdl,
        uint32_t scene_index,
        const std::function<void(const tinygltf::Node)>& f)
{
    const auto& scene = mdl.scenes.at(scene_index);
    for (const auto node : scene.nodes) {
        for_each_node(mdl, node, f);
    }
}