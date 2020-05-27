

#include "gl_scene_builder.hpp"

#include <third/tinygltf/tiny_gltf.h>

#include <assimp_handlers.hpp>


gltf::gl_scene_builder::gl_scene_builder(
    std::unique_ptr<mesh_builder> mesh_builder,
    std::unique_ptr<material_builder> material_builder,
    std::unique_ptr<parameters_builder> parameters_builder,
    std::unique_ptr<images_builder> images_builder,
    std::unique_ptr<drawables_builder> drawables_builder,
    std::unique_ptr<commands_builder> commands_builder)
    : m_mesh_builder(std::move(mesh_builder))
    , m_material_builder(std::move(material_builder))
    , m_params_builder(std::move(parameters_builder))
    , m_images_builder(std::move(images_builder))
    , m_drawables_builder(std::move(drawables_builder))
    , m_commands_builder(std::move(commands_builder))
{
}


void gltf::gl_scene_builder::build_scene(
    gl::scene::scene& gl_scene,
    const std::vector<mesh>& meshes,
    const std::vector<skin>& skins,
    tinygltf::Model& model,
    const std::string& env_texture_path)
{
    make_images(gl_scene, model);
    make_meshes(gl_scene, meshes);
    make_materials(gl_scene, model, meshes);
    make_environment(gl_scene, env_texture_path);
    m_drawables_builder->make_drawables(gl_scene, meshes);
    m_commands_builder->make_render_commands(gl_scene);
}


void gltf::gl_scene_builder::make_meshes(
    gl::scene::scene& gl_scene,
    const std::vector<mesh>& meshes)
{
    for (const auto& curr_mesh : meshes) {
        m_mesh_builder->make_mesh(curr_mesh, gl_scene);
    }
}


void gltf::gl_scene_builder::make_materials(
    gl::scene::scene& gl_scene,
    const tinygltf::Model& model,
    const std::vector<mesh>& meshes)
{
    for (const auto& mesh : meshes) {
        for (const auto& subset : mesh.get_geom_subsets()) {
            const auto mat_index = m_material_builder->make_material(gl_scene, model, subset);
            m_params_builder->make_parameters(gl_scene, gl_scene.materials.at(mat_index), subset);
        }
    }
}


void gltf::gl_scene_builder::make_images(gl::scene::scene& gl_scene, const tinygltf::Model& model)
{
    for (const auto& img : model.images) {
        m_images_builder->make_image(gl_scene, img);
    }
}


void gltf::gl_scene_builder::make_environment(
    gl::scene::scene& gl_scene,
    const std::string& env_texture_path)
{
    auto env_tex = loader::load_tex_cube(env_texture_path);
    auto ibl_diff = loader::load_diff_ibl(env_tex);
    auto [ibl_spec, ibl_brdf] = loader::load_spec_ibl(env_tex);

    gl_scene.textures.emplace_back(std::move(env_tex));
    const auto env_index = gl_scene.textures.size() - 1;
    gl_scene.textures.emplace_back(std::move(ibl_diff));
    const auto ibl_diff_index = gl_scene.textures.size() - 1;
    gl_scene.textures.emplace_back(std::move(ibl_spec));
    const auto ibl_spec_index = gl_scene.textures.size() - 1;
    gl_scene.textures.emplace_back(std::move(ibl_brdf));
    const auto ibl_brdf_index = gl_scene.textures.size() - 1;

    for (auto& mat : gl_scene.materials) {
        mat.add_texture("s_env", env_index);
        mat.add_texture("s_ibl_diff", ibl_diff_index);
        mat.add_texture("s_ibl_spec", ibl_spec_index);
        mat.add_texture("s_brdf", ibl_brdf_index);
    }
}
