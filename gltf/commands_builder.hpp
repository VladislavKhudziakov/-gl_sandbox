

#pragma once

#include <gl/scene/scene.hpp>

namespace gltf
{
    class commands_builder
    {
    public:
        virtual ~commands_builder() = default;
        virtual void make_render_commands(gl::scene::scene& scene) = 0;
    };
} // namespace gltf