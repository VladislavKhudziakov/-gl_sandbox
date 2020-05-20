

#include "mesh.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <gltf/misc/acessor_utils.hpp>

#include <third/tinygltf/tiny_gltf.h>

gltf::mesh::mesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh, int32_t skin_index)
    : m_skin_index(skin_index)
{
    for (const auto& primitive : mesh.primitives) {
        auto& curr_subset = m_geometry_subsets.emplace_back(primitive, model);
    }
}

const std::vector<gltf::mesh::geom_subset>&
gltf::mesh::get_geom_subsets() const
{
    return m_geometry_subsets;
}

gltf::mesh::geom_subset::geom_subset(const tinygltf::Primitive& primitive, const tinygltf::Model& model)
    : material_index(primitive.material)
    , topo(static_cast<mesh::topo>(primitive.mode))
{
    utils::copy_buffer_data([](glm::vec3& ref) { return glm::value_ptr(ref); }, positions, model, primitive.attributes.at("POSITION"));

    utils::copy_buffer_data([](glm::vec3& ref) { return glm::value_ptr(ref); }, normals, model, primitive.attributes.at("NORMAL"));

    utils::copy_buffer_data([](glm::vec4& ref) { return glm::value_ptr(ref); }, tangents, model, primitive.attributes.at("TANGENT"));

    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
        utils::copy_buffer_bytes(tex_coords0, model, primitive.attributes.at("TEXCOORD_0"));
    }

    if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end()) {
        utils::copy_buffer_bytes(tex_coords1, model, primitive.attributes.at("TEXCOORD_1"));
    }

    if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
        utils::copy_buffer_bytes(vertices_colors, model, primitive.attributes.at("COLOR_0"));
    }

    if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
        utils::copy_buffer_bytes(joints, model, primitive.attributes.at("JOINTS_0"));
    }

    if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
        utils::copy_buffer_bytes(weights, model, primitive.attributes.at("WEIGHTS_0"));
    }

    if (primitive.indices >= 0) {
        utils::copy_buffer_bytes(indices, model, primitive.indices);
    }
}
