

#pragma once

#include <gltf/gltf_graph.hpp>

#include <glm/mat4x4.hpp>

#include <string>
#include <vector>

namespace tinygltf
{
    class Skin;
    class Model;
} // namespace tinygltf

namespace gltf
{
    class skin
    {
    public:
        struct animation
        {
            std::string name;
            std::vector<glm::mat4> keys;
        };

        skin(const tinygltf::Model& model, const tinygltf::Skin& skin, const scene_graph& graph);
        ~skin() = default;
        const std::string& get_name() const;
        const std::vector<glm::mat4>& get_nodes_matrices() const;
        const std::vector<std::shared_ptr<scene_graph::node>>& get_nodes() const;

        std::vector<animation> animations;

    private:
        std::string m_name;
        std::vector<glm::mat4> m_inv_bind_poses;
        std::vector<std::shared_ptr<scene_graph::node>> m_nodes;
    };


} // namespace gltf
