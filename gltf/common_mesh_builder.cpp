

#include "common_mesh_builder.hpp"

#include <gltf/misc/gl_vao_utils.hpp>


void gltf::common_mesh_builder::make_mesh(const gltf::mesh& mesh, gl::scene::scene& scene)
{
    for (const auto& geom_subset : mesh.get_geom_subsets()) {
        make_subset(scene, geom_subset);
    }
}


std::unique_ptr<gltf::common_mesh_builder> gltf::common_mesh_builder::create()
{
    return std::make_unique<gltf::common_mesh_builder>();
}


void gltf::common_mesh_builder::make_subset(gl::scene::scene& gl_scene, const gltf::mesh::geom_subset geom_subset)
{
    auto& vao = gl_scene.vertex_sources.emplace_back();

    if (!geom_subset.positions.data.empty()) {
        utils::fill_vao(geom_subset.positions, vao, 0);
    }

    if (!geom_subset.tex_coords0.data.empty()) {
        utils::fill_vao(geom_subset.tex_coords0, vao, 1);
    }

    if (!geom_subset.normals.data.empty()) {
        utils::fill_vao(geom_subset.normals, vao, 2);
    }

    if (!geom_subset.tangents.data.empty()) {
        utils::fill_vao(geom_subset.tangents, vao, 3);
    }

    if (!geom_subset.joints.data.empty()) {
        utils::fill_vao(geom_subset.joints, vao, 4);
    }

    if (!geom_subset.weights.data.empty()) {
        utils::fill_vao(geom_subset.weights, vao, 5);
    }

    if (!geom_subset.tex_coords1.data.empty()) {
        utils::fill_vao(geom_subset.tex_coords1, vao, 6);
    }

    if (!geom_subset.vertices_colors.data.empty()) {
        utils::fill_vao(geom_subset.vertices_colors, vao, 7);
    }

    uint32_t i_size = 0;
    gl::scene::mesh::indices_type i_type = gl::scene::mesh::indices_type::none;

    if (geom_subset.indices.data.size() > 0) {
        gl::buffer<GL_ELEMENT_ARRAY_BUFFER> ebo;
        ebo.fill(geom_subset.indices.data.data(), geom_subset.indices.data.size());

        vao.bind();
        ebo.bind();
        vao.unbind();
        ebo.unbind();

        const auto el_size = utils::get_element_size(geom_subset.indices.c_type);
        const auto el_count = utils::get_elements_count(geom_subset.indices.d_type);
        i_type = static_cast<gl::scene::mesh::indices_type>(geom_subset.indices.c_type);
        i_size = geom_subset.indices.data.size() / (el_size * el_count);
    }

    const auto el_size = utils::get_element_size(geom_subset.positions.c_type);
    const auto el_count = utils::get_elements_count(geom_subset.positions.d_type);
    const auto pos_size = geom_subset.positions.data.size() / (el_size * el_count);

    gl_scene.meshes.emplace_back(gl_scene.vertex_sources.size() - 1, i_type, i_size, pos_size);
}
