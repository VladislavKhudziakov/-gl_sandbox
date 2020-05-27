


#pragma once

#include <gltf/parameters_builder.hpp>
#include <gl/scene/scene.hpp>

namespace gltf
{
    class common_parameters_builder : public parameters_builder
    {
    public:
        void make_parameters(gl::scene::scene& scene, gl::scene::material& material, const mesh::geom_subset& geom_subset) override;
    private:
        void make_global_params(gl::scene::scene& scene);
        bool m_globals_created {false};
        uint32_t m_projection_index;
        uint32_t m_view_index;
    };
}

