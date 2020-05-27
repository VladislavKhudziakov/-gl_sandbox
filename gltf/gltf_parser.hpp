

#pragma once

#include <gltf/meshes_processor.hpp>

#include <gl/scene/scene.hpp>

#include <gltf/gl_scene_builder.hpp>

namespace gltf
{
    class gltf_parser
    {
    public:
        gltf_parser(gl_scene_builder);
        void parse(const std::string& path, const std::string& env_path, gl::scene::scene& scene, uint32_t scene_index = 0);
    private:
        gl_scene_builder m_builder;
    };
}

