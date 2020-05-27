


#pragma once

#include <gltf/mesh.hpp>
#include <gltf/meshes_processor.hpp>

#include <gl/scene/scene.hpp>

namespace tinygltf
{
    class Model;
}

class material_builder
{
public:
    virtual ~material_builder() = default;
    virtual uint32_t make_material(gl::scene::scene& gl_scene, const tinygltf::Model& model, const gltf::mesh::geom_subset& subset) = 0;
};
