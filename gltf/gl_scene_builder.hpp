


#pragma once

#include <gl/scene/scene.hpp>
#include <gltf/meshes_processor.hpp>
#include <gltf/mesh_builder.hpp>
#include <gltf/material_builder.hpp>
#include <gltf/parameters_builder.hpp>
#include <gltf/images_builder.hpp>
#include <gltf/drawables_builder.hpp>
#include <gltf/commands_builder.hpp>




#include <unordered_map>

namespace tinygltf
{
    class Model;
}

namespace gltf
{
    class gl_scene_builder
    {
    public:
        gl_scene_builder(
            std::unique_ptr<mesh_builder>,
            std::unique_ptr<material_builder>,
            std::unique_ptr<parameters_builder>,
            std::unique_ptr<images_builder>,
            std::unique_ptr<drawables_builder>,
            std::unique_ptr<commands_builder>);

        void build_scene(
            gl::scene::scene& gl_scene,
            const std::vector<mesh>& meshes,
            const std::vector<skin>& skins,
            tinygltf::Model& model,
            const std::string& env_texture_path);

    private:
        void make_images(gl::scene::scene& gl_scene, const tinygltf::Model& model);
        void make_meshes(gl::scene::scene& gl_scene, const std::vector<mesh>& meshes);
        void make_materials(gl::scene::scene& gl_scene, const tinygltf::Model& model, const std::vector<mesh>& meshes);
        void make_environment(gl::scene::scene& gl_scene, const std::string& env_texture_path);

        std::unique_ptr<mesh_builder> m_mesh_builder;
        std::unique_ptr<material_builder> m_material_builder;
        std::unique_ptr<parameters_builder> m_params_builder;
        std::unique_ptr<images_builder> m_images_builder;
        std::unique_ptr<drawables_builder> m_drawables_builder;
        std::unique_ptr<commands_builder> m_commands_builder;

    };
}

