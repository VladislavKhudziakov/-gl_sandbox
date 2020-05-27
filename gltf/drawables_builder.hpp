

#pragma once

#include <gl/scene/scene.hpp>
#include <gltf/mesh.hpp>

namespace gltf
{
    class drawables_builder
    {
    public:
        virtual ~drawables_builder() = default;
        virtual void make_drawables(gl::scene::scene& gl_scene, const std::vector<mesh>& meshes) = 0;
    };
}

