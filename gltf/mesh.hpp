

#pragma once

#include <gltf/misc/data_storage.hpp>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <vector>
#include <string>
#include <array>


namespace tinygltf
{
  class Mesh;
  class Model;
  class Primitive;
  class Material;
}


namespace gltf
{
    class mesh
    {
    public:
        enum class topo
        {
            points,
            lines,
            lines_loop,
            lines_strip,
            triangles,
            triangles_strip,
            triangles_fan,
            lines_adj = 0x000A,
            triangles_adj = 0x000C
        };

        struct geom_subset
        {
            geom_subset(const tinygltf::Primitive& primitive, const tinygltf::Model& model);
            ~geom_subset() = default;

            mesh::topo topo;

            data_storage positions;
            data_storage normals;
            data_storage tangents;
            data_storage tex_coords0;
            data_storage tex_coords1;
            data_storage vertices_colors;
            data_storage joints;
            data_storage weights;
            data_storage indices;

            uint32_t material;
        };

        mesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh, int32_t skin_index);
        virtual ~mesh() = default;
        const std::vector<geom_subset>& get_geom_subsets() const;

    private:
        int32_t m_skin_index;
        std::vector<geom_subset> m_geometry_subsets;
    };
}
