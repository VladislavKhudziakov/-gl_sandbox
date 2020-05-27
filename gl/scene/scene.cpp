

#include "scene.hpp"

#include <variant>

namespace gl
{
    template<>
    class uniform_resolver<glm::ivec2>
    {
    public:
        void operator()(uint32_t location, const glm::ivec2& v, size_t count = 1)
        {
            glUniform2iv(location, count, glm::value_ptr(v));
        }
    };

    template<>
    class uniform_resolver<glm::ivec3>
    {
    public:
        void operator()(uint32_t location, const glm::ivec3& v, size_t count = 1)
        {
            glUniform3iv(location, count, glm::value_ptr(v));
        }
    };

    template<>
    class uniform_resolver<glm::vec4>
    {
    public:
        void operator()(uint32_t location, const glm::vec4& v, size_t count = 1)
        {
            glUniform4fv(location, count, glm::value_ptr(v));
        }
    };

    template<>
    class uniform_resolver<glm::ivec4>
    {
    public:
        void operator()(uint32_t location, const glm::ivec4& v, size_t count = 1)
        {
            glUniform4iv(location, count, glm::value_ptr(v));
        }
    };

    template<>
    class uniform_resolver<glm::mat2>
    {
    public:
        void operator()(uint32_t location, const glm::mat2& v, size_t count = 1)
        {
            glUniformMatrix2fv(location, count, GL_FALSE, glm::value_ptr(v));
        }
    };

    template<>
    class uniform_resolver<glm::mat3>
    {
    public:
        void operator()(uint32_t location, const glm::mat2& v, size_t count = 1)
        {
            glUniformMatrix3fv(location, count, GL_FALSE, glm::value_ptr(v));
        }
    };
} // namespace gl

namespace
{
    void set_shader_param(const gl::program& p, const std::string& param_name, void* data, gl::scene::parameter_type pt, gl::scene::parameter_component_type pct)
    {
        switch (pct) {
            case gl::scene::parameter_component_type::vec1:
                if (pt == gl::scene::parameter_type::f32) {
                    p.set_uniform(param_name, *(float*) (data));
                } else {
                    p.set_uniform(param_name, *(int32_t*) (data));
                }
                break;
            case gl::scene::parameter_component_type::vec2:
                if (pt == gl::scene::parameter_type::f32) {
                    p.set_uniform(param_name, *(glm::vec2*) (data));
                } else {
                    p.set_uniform(param_name, *(glm::ivec2*) (data));
                }
                break;
            case gl::scene::parameter_component_type::vec3:
                if (pt == gl::scene::parameter_type::f32) {
                    p.set_uniform(param_name, *(glm::vec3*) (data));
                } else {
                    p.set_uniform(param_name, *(glm::ivec3*) (data));
                }
                break;
            case gl::scene::parameter_component_type::vec4:
                if (pt == gl::scene::parameter_type::f32) {
                    p.set_uniform(param_name, *(glm::vec4*) (data));
                } else {
                    p.set_uniform(param_name, *(glm::ivec4*) (data));
                }
                break;
            case gl::scene::parameter_component_type::mat2:
                if (pt == gl::scene::parameter_type::f32) {
                    p.set_uniform(param_name, *(glm::mat2*) (data));
                    break;
                } else {
                    [[fallthrough]];
                }
            case gl::scene::parameter_component_type::mat3:
                if (pt == gl::scene::parameter_type::f32) {
                    p.set_uniform(param_name, *(glm::mat3*) (data));
                    break;
                } else {
                    [[fallthrough]];
                }
            case gl::scene::parameter_component_type::mat4:
                if (pt == gl::scene::parameter_type::f32) {
                    p.set_uniform(param_name, *(glm::mat4*) (data));
                    break;
                } else {
                    [[fallthrough]];
                }
            default:
                throw std::runtime_error("unsupported type");
        }
    }
} // namespace

void gl::scene::draw(
    const gl::scene::scene& s,
    const std::vector<uint32_t>& mesh_instances,
    uint32_t pass_idx)
{
    const auto& pass = s.passes.at(pass_idx);
    const auto& framebuffer = s.framebuffers.at(pass.get_framebuffer_idx());

    gl::bind_guard pass_guard(pass);

    for (const auto i : mesh_instances) {
        const auto& drawable = s.drawables.at(i);
        const auto& mesh = s.meshes.at(drawable.mesh_idx);
        const auto& mat = s.materials.at(drawable.material_idx);
        const auto& shader = s.shaders.at(mat.get_program());

        gl::bind_guard g_vertices(s.vertex_sources.at(mesh.get_vertices()));

        gl::bind_guard g_mat(shader);

        int32_t slot = 0;

        for (const auto& [s_name, s_idx] : mat.get_textures()) {
            const auto& sampler_name = s_name;
            std::visit([&sampler_name, &slot, &shader](auto& t) {
                t.bind(slot);
                shader.set_uniform(sampler_name, slot);
            },
                       s.textures.at(s_idx));
            ++slot;
        }


        for (const auto& [p_name, p_index] : mat.get_parameters()) {
            const auto& parameter = s.parameters.at(p_index);
            auto param_data = const_cast<uint8_t*>(parameter.get_data());
            set_shader_param(shader, p_name, param_data, parameter.get_param_type(), parameter.get_component_type());
        }

        pass.set_state(mat.get_state());

        if (mesh.get_indices_size() > 0) {
            glDrawElements(GLenum(drawable.topo), mesh.get_indices_size(), GLenum(mesh.get_indices_type()), nullptr);
        } else {
            glDrawArrays(GLenum(drawable.topo), 0, mesh.get_vertices_size());
        }

        for (const auto& [s_name, s_idx] : mat.get_textures()) {
            std::visit([](auto& t) { t.unbind(); }, s.textures.at(s_idx));
        }
    }
}


void gl::scene::draw(const gl::scene::scene& s, uint32_t surface_width, uint32_t surface_height)
{
    const gl::scene::pass* curr_pass = nullptr;

    for (const auto& command : s.commands) {
        switch (command.type) {
            case render_command::type::pass:
                if (curr_pass) {
                    curr_pass->unbind();
                }
                curr_pass = &s.passes.at(command.source_index);
                curr_pass->bind();
                break;
            case render_command::type::draw:
                {
                    const auto& drawable = s.drawables.at(command.source_index);
                    const auto& mesh = s.meshes.at(drawable.mesh_idx);
                    const auto& mat = s.materials.at(drawable.material_idx);
                    const auto& shader = s.shaders.at(mat.get_program());
                    gl::bind_guard g_vertices(s.vertex_sources.at(mesh.get_vertices()));
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

                    for (const auto& [p_name, p_index] : mat.get_parameters()) {
                        const auto& parameter = s.parameters.at(p_index);
                        auto param_data = const_cast<uint8_t*>(parameter.get_data());
                        set_shader_param(shader, p_name, param_data, parameter.get_param_type(), parameter.get_component_type());
                    }

                    curr_pass->set_state(mat.get_state());

                    if (mesh.get_indices_size() > 0) {
                        glDrawElements(GLenum(drawable.topo), mesh.get_indices_size(), GLenum(mesh.get_indices_type()), nullptr);
                    } else {
                        glDrawArrays(GLenum(drawable.topo), 0, mesh.get_vertices_size());
                    }

                    for (const auto& [s_name, s_idx] : mat.get_textures()) {
                        std::visit([](auto& t) { t.unbind(); }, s.textures.at(s_idx));
                    }
                }
                break;
            case render_command::type::blit:
                {
                    const auto& src_pass = s.passes.at(command.source_index);
                    const auto& dst_pass = s.passes.at(command.dst_index);
                    const auto& src_fb = s.framebuffers.at(src_pass.get_framebuffer_idx());
                    const auto& dst_fb = s.framebuffers.at(dst_pass.get_framebuffer_idx());
                    src_fb.blit(dst_fb);
                }
                break;
        }
    }

    assert(glGetError() == GL_NO_ERROR);

    if (curr_pass) {
        const auto& curr_fb = s.framebuffers.at(curr_pass->get_framebuffer_idx());
        curr_fb.blit(surface_width, surface_height);
        curr_pass->unbind();
    }
}
