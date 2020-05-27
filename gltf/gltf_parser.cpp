

#include "gltf_parser.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <third/tinygltf/tiny_gltf.h>


void gltf::gltf_parser::parse(const std::string& path, const std::string& env_path, gl::scene::scene& gl_scene, uint32_t scene_index)
{
    tinygltf::Model mdl;
    tinygltf::TinyGLTF loader;

    std::string err_msg;
    std::string warn_msg;

    bool load_success = false;

    const auto ext = path.substr(path.find_last_of(".") + 1);

    const bool is_glb = ext == "glb";
    const bool is_gltf = ext == "gltf";

    assert(is_glb || is_gltf);

    if (is_gltf) {
        load_success = loader.LoadASCIIFromFile(&mdl, &err_msg, &warn_msg, path);
    } else {
        load_success = loader.LoadBinaryFromFile(&mdl, &err_msg, &warn_msg, path);
    }

    if (!load_success) {
        throw std::runtime_error(err_msg);
    }

    gltf::meshes_processor mesh_processor;
    mesh_processor.m_model = &mdl;
    mesh_processor.process_meshes(scene_index);
    m_builder.build_scene(gl_scene, mesh_processor.get_meshes(), mesh_processor.get_skins(), mdl, env_path);
}


gltf::gltf_parser::gltf_parser(gltf::gl_scene_builder b)
    : m_builder(std::move(b))
{
}
