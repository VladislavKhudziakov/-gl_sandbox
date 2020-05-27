


#pragma once

#include <gltf/mesh.hpp>
#include <gl/scene/scene.hpp>

namespace gltf
{
    class mesh_builder
    {
    public:
        virtual ~mesh_builder() = default;
        virtual void make_mesh(const gltf::mesh&, gl::scene::scene&) = 0;
    };
}

