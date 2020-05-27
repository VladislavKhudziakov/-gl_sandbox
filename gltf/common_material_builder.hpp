


#pragma once

#include <gltf/material_builder.hpp>

namespace gltf
{
    class common_material_builder : public material_builder
    {
    public:
        uint32_t make_material(gl::scene::scene& gl_scene, const tinygltf::Model& model, const mesh::geom_subset& subset) override;
    };
}

