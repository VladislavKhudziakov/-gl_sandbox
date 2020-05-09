

#include "scene.hpp"

#include <variant>

#include <iostream>

void gl::scene::draw(
    const gl::scene::scene &s,
    const std::vector<uint32_t>& nodes,
    uint32_t pass_idx)
{
  const auto& pass = s.passes.at(pass_idx);
  const auto& framebuffer = s.framebuffers.at(pass.get_framebuffer_idx());
  const auto [w, h] = framebuffer.get_size();

  auto projection = glm::perspectiveFov(s.camera.fov, float(w), float(h), s.camera.near, s.camera.far);

  auto view = glm::lookAt(s.camera.position, s.camera.forward, s.camera.up);

  gl::bind_guard pass_guard(pass);

  for (const auto i : nodes) {
    const auto& n = s.nodes.at(i);
    const auto& mesh = s.meshes.at(n.mesh_idx);
    const auto& mat = s.materials.at(n.material_idx);
    const auto& shader = s.shaders.at(mat.get_program());
    gl::bind_guard g_vertices(s.vertex_sources.at(mesh.get_vertices()));
    std::unique_ptr<gl::bind_guard<gl::buffer<GL_ELEMENT_ARRAY_BUFFER>>> g_indices;
    if (mesh.get_indices() >= 0) {
      g_indices = std::make_unique<gl::bind_guard<gl::buffer<GL_ELEMENT_ARRAY_BUFFER>>>(s.index_sources.at(mesh.get_indices()));
    }

    gl::bind_guard g_mat(shader);

    int32_t slot = 0;

    for (const auto& [s_name, s_idx] : mat.get_textures()) {
      const auto& sampler_name = s_name;
      std::visit([&sampler_name, &slot, &shader](auto& t) {
        t.bind(slot);
        shader.set_uniform(sampler_name, slot);
      }, s.textures.at(s_idx));
      ++slot;
    }

    auto translation = glm::translate(glm::mat4{1}, n.transformation.translation);
    auto scale = glm::scale(glm::mat4{1}, n.transformation.scale);
    auto rotation_x = glm::rotate(glm::mat4{1}, n.transformation.rotation.x, {1.f, 0.f, 0.f});
    auto rotation_y = glm::rotate(glm::mat4{1}, n.transformation.rotation.y, {0.f, 1.f, 0.f});
    auto rotation_z = glm::rotate(glm::mat4{1}, n.transformation.rotation.z, {0.f, 0.f, 1.f});
    auto rotation = rotation_x * rotation_y * rotation_z;

    auto model = translation * rotation * scale;

    auto mvp = projection * view * model;

    shader.set_uniform("u_MVP", mvp);
    shader.set_uniform("u_PROJECTION", projection);
    shader.set_uniform("u_VIEW", view);
    shader.set_uniform("u_MODEL", model);

    pass.set_state(mat.get_state());

    if (mesh.get_indices() >= 0) {
      glDrawElements(GL_TRIANGLES, mesh.get_indices_size(), mesh.get_indices_type(), nullptr);
    } else {
      glDrawArrays(GL_TRIANGLES, 0, mesh.get_vertices_size());
    }

    for (const auto& [s_name, s_idx] : mat.get_textures()) {
      std::visit([](auto& t) { t.unbind(); }, s.textures.at(s_idx));
    }
  }
}
