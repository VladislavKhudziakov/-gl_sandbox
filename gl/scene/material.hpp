

#pragma once

#include <gl_handlers.hpp>
#include <gl/scene/pass.hpp>

#include <variant>
#include <unordered_map>


namespace gl::scene
{
    using texture = std::variant<gl::texture<GL_TEXTURE_2D>, gl::texture<GL_TEXTURE_CUBE_MAP>, gl::texture<GL_TEXTURE_1D>, gl::texture<GL_TEXTURE_3D>>;

    class material
    {
    public:
        explicit material(uint32_t program_idx);
        material(material&&) = default;
        material& operator=(material&&) = default;
        ~material() = default;

        void set_state(gpu_state);
        const gpu_state& get_state() const;

        uint32_t get_program() const;

        void add_texture(const std::string&, uint32_t);
        void add_parameter(std::string, uint32_t);
        const std::unordered_map<std::string, uint32_t>& get_textures() const;
        const std::unordered_map<std::string, uint32_t>& get_parameters() const;

    private:
        uint32_t m_program;
        std::unordered_map<std::string, uint32_t> m_textures;
        std::unordered_map<std::string, uint32_t> m_parameters;
        gl::scene::gpu_state m_state{};
    };
} // namespace gl::scene
