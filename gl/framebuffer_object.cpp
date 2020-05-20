

#include "framebuffer_object.hpp"

gl::framebuffer_object::framebuffer_object()
    : m_gl_handler(0)
{
    glGenFramebuffers(1, &m_gl_handler);
}


gl::framebuffer_object::~framebuffer_object()
{
    if (m_gl_handler > 0) {
        glDeleteFramebuffers(1, &m_gl_handler);
    }
}


gl::framebuffer_object::framebuffer_object(gl::framebuffer_object&& src) noexcept
    : m_gl_handler(0)
{
    *this = std::move(src);
}


gl::framebuffer_object& gl::framebuffer_object::operator=(gl::framebuffer_object&& src) noexcept
{
    if (this != &src) {
        m_gl_handler = src.m_gl_handler;
        src.m_gl_handler = 0;
    }

    return *this;
}


void gl::framebuffer_object::bind(GLenum target)
{
    m_bound_target = target;
    glBindFramebuffer(m_bound_target, m_gl_handler);
}


void gl::framebuffer_object::unbind()
{
    assert(m_bound_target != GLenum(-1));
    glBindFramebuffer(m_bound_target, 0);
    m_bound_target = GLenum(-1);
}


void gl::framebuffer_object::blit(
    uint32_t dst_handler, uint32_t from_width, uint32_t from_height, uint32_t dst_width, uint32_t dst_height)
{
    int32_t read_fb, draw_fb;

    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fb);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fb);

    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_gl_handler);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_handler);
        glBlitFramebuffer(0, 0, from_width, from_height, 0, 0, dst_width, dst_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fb);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fb);
}


gl::framebuffer_object::operator uint32_t() const
{
    return m_gl_handler;
}
