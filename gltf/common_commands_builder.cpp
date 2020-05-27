

#include "common_commands_builder.hpp"


void gltf::common_commands_builder::make_render_commands(gl::scene::scene& scene)
{
    scene.textures.emplace_back(gl::texture<GL_TEXTURE_2D>{}); // color attachment texture
    const auto color_attachment_idx = scene.textures.size() - 1;
    scene.textures.emplace_back(gl::texture<GL_TEXTURE_2D>{}); // depth attachment texture
    const auto depth_attachment_idx = scene.textures.size() - 1;

    auto& a = scene.attachments.emplace_back(scene, color_attachment_idx, gl::scene::attachment_type::rgba8);
    float clear_vals[] {0, 1, 0, 1};
    a.set_clear_values(clear_vals);
    scene.attachments.emplace_back(scene, depth_attachment_idx, gl::scene::attachment_type::depth24f);

    scene.fbos.emplace_back();

    auto& fb = scene.framebuffers.emplace_back(scene, scene.fbos.size() - 1, 1600, 1200);

    fb.add_attachment(fb.color1, 0);
    fb.add_attachment(fb.depth, 1);

    scene.passes.emplace_back(scene, scene.framebuffers.size() - 1);

    scene.commands.emplace_back(gl::scene::render_command {gl::scene::render_command::type::pass, 0});

    for (uint32_t i = 0; i < scene.drawables.size(); ++i) {
        scene.commands.emplace_back(gl::scene::render_command {gl::scene::render_command::type::draw, i});
    }
}
