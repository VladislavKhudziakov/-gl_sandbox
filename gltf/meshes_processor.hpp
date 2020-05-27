

#pragma once

#include <gltf/gltf_graph.hpp>

#include <gltf/skin.hpp>
#include <gltf/mesh.hpp>


namespace gltf
{
    class meshes_processor
    {
        friend class gltf_parser;
    public:
        meshes_processor() = default;
        std::shared_ptr<scene_graph> get_graph() const;
        const std::vector<skin>& get_skins() const;
        const std::vector<mesh>& get_meshes() const;

    private:
        void process_meshes(uint32_t scene_index);
        void calculate_animations();

        std::shared_ptr<scene_graph> m_graph;
        tinygltf::Model* m_model;
        std::vector<skin> m_skins;
        std::vector<mesh> m_meshes;
    };
} // namespace gltf
