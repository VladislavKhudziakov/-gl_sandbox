

#pragma once

#include <gltf/misc/data_storage.hpp>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <vector>

namespace tinygltf
{
  class Mesh;
  class Model;
  class Primitive;
}

namespace gltf
{
    class mesh
    {
    public:
        enum class topo
        {
          points, lines, lines_loop, lines_strip, triangles, triangles_strip, triangles_fan
        };

        struct geom_subset
        {
            geom_subset(const tinygltf::Primitive& primitive, const tinygltf::Model& model);
            ~geom_subset() = default;

            uint32_t material_index;
            mesh::topo topo;

            std::vector<glm::vec3> positions;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec4> tangents;

            data_storage tex_coords0;
            data_storage tex_coords1;
            data_storage vertices_colors;
            data_storage joints;
            data_storage weights;
            data_storage indices;
        };

        mesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh, int32_t skin_index);
        virtual ~mesh() = default;
        const std::vector<geom_subset>& get_geom_subsets() const;

    protected:
        int32_t m_skin_index;
        std::vector<geom_subset> m_geometry_subsets;
    };
}
