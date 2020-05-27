


#pragma once

#include <gltf/mesh_builder.hpp>
#include <gltf/material_builder.hpp>

namespace gltf
{
    class common_mesh_builder : public mesh_builder
    {
    public:
        static std::unique_ptr<common_mesh_builder> create();
        common_mesh_builder() = default;
        ~common_mesh_builder() override = default;
        void make_mesh(const mesh& mesh, gl::scene::scene& scene) override;
    private:
        void make_subset(gl::scene::scene&, const mesh::geom_subset subset);
    };
}

