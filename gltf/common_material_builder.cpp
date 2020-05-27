

#include "common_material_builder.hpp"

#include <shaders.hpp>
#include <assimp_handlers.hpp>

#include <third/tinygltf/tiny_gltf.h>

uint32_t gltf::common_material_builder::make_material(
    gl::scene::scene& gl_scene,
    const tinygltf::Model& model,
    const gltf::mesh::geom_subset& subset)
{
    auto get_image = [&model](auto& info) {
        return info.index >= 0 ? model.textures.at(info.index).source : -1;
    };

    auto add_texture = [&get_image](const std::string& sampler_name, gl::scene::material& mat, auto& tex_info) {
        const auto image_index = get_image(tex_info);
        if (image_index >= 0) {
            mat.add_texture(sampler_name, image_index);
        }
    };

    const auto& mat = model.materials.at(subset.material);

    gl_scene.shaders.emplace_back(gl::shader<GL_VERTEX_SHADER>{shaders::pbr_vss}, gl::shader<GL_FRAGMENT_SHADER>{shaders::pbr_fss_ibl});
    auto& gl_mat = gl_scene.materials.emplace_back(gl_scene.shaders.size() - 1);

    gl_mat.set_state({
        {true, true, true, true},
            true,
            gl::scene::depth_func::leq,
            gl::scene::cull_func::back,
        });

    add_texture("s_albedo", gl_mat, mat.pbrMetallicRoughness.baseColorTexture);
    add_texture("s_metallic_roughness", gl_mat, mat.pbrMetallicRoughness.metallicRoughnessTexture);
    add_texture("s_normal", gl_mat, mat.normalTexture);
    add_texture("s_emissve", gl_mat, mat.emissiveTexture);
    add_texture("s_occlusion", gl_mat, mat.occlusionTexture);

    return gl_scene.materials.size() - 1;
}
