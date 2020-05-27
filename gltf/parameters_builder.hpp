


#pragma once

#include <gl/scene/material.hpp>

#include <gltf/mesh.hpp>

namespace gltf
{
    class parameters_builder
    {
    public:
        virtual ~parameters_builder() = default;
        virtual void make_parameters(gl::scene::scene& scene, gl::scene::material& material, const gltf::mesh::geom_subset& geom_subset) = 0;
    };
}

