

#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <gl/scene/scene.hpp>


gltf::camera::camera(uint32_t proj, uint32_t view, gl::scene::scene& s)
    : m_scene_proj_idx(proj)
    , m_scene_view_idx(view)
    , m_scene(s)
{
}


void gltf::camera::update(float w, float h)
{
    m_proj_matrix = glm::perspectiveFov(m_fov, w, h, m_near, m_far);
    m_view_matrix = glm::lookAt(m_position, m_direction, {0, 1, 0});

    auto& proj_param = m_scene.parameters.at(m_scene_proj_idx);
    auto& view_param = m_scene.parameters.at(m_scene_view_idx);

    std::memcpy(proj_param.get_data(), glm::value_ptr(m_proj_matrix), sizeof(m_proj_matrix));
    std::memcpy(view_param.get_data(), glm::value_ptr(m_view_matrix), sizeof(m_proj_matrix));
}
