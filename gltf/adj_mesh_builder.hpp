

#pragma once

#include <gltf/mesh_builder.hpp>


namespace gltf
{
    class adj_mesh_builder : public mesh_builder
    {
    public:
        void make_mesh(const mesh& mesh, gl::scene::scene& scene) override;
    private:
        void make_subset(gl::scene::scene& gl_scene, const gltf::mesh::geom_subset geom_subset);
    };
}

