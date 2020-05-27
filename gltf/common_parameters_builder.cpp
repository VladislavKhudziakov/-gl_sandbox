

#include "common_parameters_builder.hpp"



void gltf::common_parameters_builder::make_global_params(gl::scene::scene& scene)
{
    if (!m_globals_created) {
        scene.parameters.emplace_back(gl::scene::parameter_type::f32, gl::scene::parameter_component_type::mat4);
        m_projection_index = scene.parameters.size() - 1;
        scene.parameters.emplace_back(gl::scene::parameter_type::f32, gl::scene::parameter_component_type::mat4);
        m_view_index = scene.parameters.size() - 1;
        m_globals_created = true;
    }
}

void gltf::common_parameters_builder::make_parameters(
    gl::scene::scene& scene,
    gl::scene::material& material,
    const gltf::mesh::geom_subset& geom_subset)
{
    make_global_params(scene);

    scene.parameters.emplace_back(gl::scene::parameter_type::f32, gl::scene::parameter_component_type::mat4);
    const auto mvp_index = scene.parameters.size() - 1;
    scene.parameters.emplace_back(gl::scene::parameter_type::f32, gl::scene::parameter_component_type::mat4);
    const auto model_index = scene.parameters.size() - 1;
    scene.parameters.emplace_back(gl::scene::parameter_type::f32, gl::scene::parameter_component_type::vec1);
    const auto anim_key_index = scene.parameters.size() - 1;

    material.add_parameter("u_MVP", mvp_index);
    material.add_parameter("u_MODEL", model_index);
    material.add_parameter("u_ANIM_KEY", anim_key_index);
    material.add_parameter("u_PROJECTION", m_projection_index);
    material.add_parameter("u_VIEW", m_view_index);
}
