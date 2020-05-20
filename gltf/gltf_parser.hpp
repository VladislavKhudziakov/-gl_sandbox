

#pragma once

#include <gltf/gltf_graph.hpp>

#include <gltf/skin.hpp>
#include <gltf/mesh.hpp>


namespace gltf
{
    class gltf_parser
    {
    public:
        gltf_parser(const tinygltf::Model& mdl);
        scene_graph& get_graph();
        const scene_graph& get_graph() const;
        const std::vector<skin>& get_skins() const;
        const std::vector<mesh>& get_meshes() const;
        void parse();

    private:
        void calculate_animations();

        scene_graph m_graph;
        const tinygltf::Model& m_model;
        std::vector<skin> m_skins;
        std::vector<mesh> m_meshes;
    };
} // namespace gltf
