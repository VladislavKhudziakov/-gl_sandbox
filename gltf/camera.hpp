

#pragma once

#include <cinttypes>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace gl::scene
{
    struct scene;
}

namespace gltf
{
    class camera
    {
    public:
        camera(uint32_t, uint32_t, gl::scene::scene&);
        ~camera() = default;
        void update(float w, float h);

        float m_fov{90.f};
        float m_near{0.01};
        float m_far{1000};

        glm::vec3 m_position{0, 0, 10};
        glm::vec3 m_direction{0, 0, -1};

        glm::mat4 m_proj_matrix;
        glm::mat4 m_view_matrix;

    private:
        uint32_t m_scene_proj_idx;
        uint32_t m_scene_view_idx;
        gl::scene::scene& m_scene;
    };
} // namespace gltf
