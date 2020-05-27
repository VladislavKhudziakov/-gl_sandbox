


#pragma once

#include <gltf/drawables_builder.hpp>

namespace gltf
{
    class common_drawables_builder : public drawables_builder
    {
    public:
        void make_drawables(gl::scene::scene& gl_scene, const std::vector<mesh>& meshes) override;

    private:
    };
}

