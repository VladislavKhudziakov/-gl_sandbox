
#include "attachment.hpp"

#include <gl/scene/scene.hpp>
#include <gl/textures.hpp>

namespace gl
{
    template<>
    struct gl::texture_fill_resolver<gl::scene::attachment, GL_TEXTURE_2D>
    {
        void operator()(void* data, int32_t w, int32_t h, gl::scene::attachment_type type)
        {
            switch (type) {
                case gl::scene::attachment_type::rgba8:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                    break;
                case gl::scene::attachment_type::rgba16f:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
                    break;
                case gl::scene::attachment_type::depth24f:
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
                    break;
            }
        }
    };
} // namespace gl


gl::scene::attachment::attachment(
    const gl::scene::scene& s,
    uint32_t i,
    gl::scene::attachment_type t)
    : m_scene(s)
    , m_texture_handler_idx(i)
    , m_type(t)
{
}


void gl::scene::attachment::resize(uint32_t w, uint32_t h) const
{
    auto& tex_handler = m_scene.textures.at(m_texture_handler_idx);
    std::get<gl::texture<GL_TEXTURE_2D>>(tex_handler).fill<gl::scene::attachment>(nullptr, w, h, m_type);
}


gl::scene::attachment_type gl::scene::attachment::get_type() const
{
    return m_type;
}


void gl::scene::attachment::set_start_pass_behaviour(gl::scene::pass_behaviour b)
{
    m_start_behaviour = b;
}


void gl::scene::attachment::set_end_pass_behaviour(gl::scene::pass_behaviour b)
{
    m_finish_behaviour = b;
}


uint32_t gl::scene::attachment::get_handler_idx() const
{
    return m_texture_handler_idx;
}


void gl::scene::attachment::set_clear_values(const float* values)
{
    for (size_t i = 0; i < 4; ++i) {
        m_clear_values[0] = values[i];
    }
}


gl::scene::pass_behaviour gl::scene::attachment::get_start_pass_behaviour() const
{
    return m_start_behaviour;
}


gl::scene::pass_behaviour gl::scene::attachment::get_finish_pass_behaviour() const
{
    return m_finish_behaviour;
}


const float* gl::scene::attachment::get_clear_values() const
{
    return m_clear_values;
}
