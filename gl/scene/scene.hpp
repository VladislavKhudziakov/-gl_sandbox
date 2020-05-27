

#pragma once

#include <gl/scene/meshes.hpp>
#include <gl/framebuffer_object.hpp>
#include <gl/scene/attachment.hpp>
#include <gl/scene/framebuffer.hpp>
#include <gl/scene/pass.hpp>
#include <gl/scene/parameter.hpp>

#include <vector>

namespace gl::scene
{
    struct drawable
    {
        enum class topology
        {
            points,
            lines,
            line_loop,
            line_strip,
            triangles,
            triangle_strip,
            triangle_fan,
            lines_adj = 0x000A,
            triangles_adj = 0x000C
        };

        uint32_t mesh_idx = -1;
        uint32_t material_idx = -1;

        drawable::topology topo = drawable::topology::triangles;
    };


    struct render_command
    {
        enum class type
        {
            pass, draw, blit
        };

        render_command::type type;

        uint32_t source_index;
        uint32_t dst_index;
    };

    struct scene
    {
        std::vector<gl::scene::mesh> meshes;
        std::vector<gl::scene::material> materials;
        std::vector<gl::scene::drawable> drawables;
        std::vector<gl::scene::attachment> attachments;
        std::vector<gl::scene::framebuffer> framebuffers;
        std::vector<gl::scene::pass> passes;
        std::vector<gl::scene::texture> textures;
        std::vector<gl::scene::parameter> parameters;
        std::vector<gl::scene::render_command> commands;

        std::vector<gl::framebuffer_object> fbos;
        std::vector<gl::program> shaders;
        std::vector<gl::vertex_array_object> vertex_sources;
    };

    void draw(const scene& s, const std::vector<uint32_t>&, uint32_t pass_idx);

    void draw(const scene& s, uint32_t surface_width, uint32_t surface_height);
} // namespace gl::scene
