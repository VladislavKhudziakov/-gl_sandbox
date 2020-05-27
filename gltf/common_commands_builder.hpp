


#pragma once

#include <gltf/commands_builder.hpp>

namespace gltf
{
    class common_commands_builder : public commands_builder
    {
    public:
        ~common_commands_builder() override = default;
        void make_render_commands(gl::scene::scene& scene) override;
    };
}