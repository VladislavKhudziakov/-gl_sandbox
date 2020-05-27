

#include "common_drawables_builder.hpp"


void gltf::common_drawables_builder::make_drawables(gl::scene::scene& gl_scene, const std::vector<mesh>& meshes)
{
    uint32_t counter = 0;
    for (const auto& mesh : meshes) {
        for (const auto subset : mesh.get_geom_subsets()) {
            auto& drawable = gl_scene.drawables.emplace_back();
            drawable.topo = static_cast<gl::scene::drawable::topology>(subset.topo);
            drawable.mesh_idx = counter;
            drawable.material_idx = counter;
            ++counter;
        }
    }
}
