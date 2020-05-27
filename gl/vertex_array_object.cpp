

#include "vertex_array_object.hpp"


gl::vertex_array_object::vertex_array_object()
{
    glGenVertexArrays(1, &m_gl_handler);
}


gl::vertex_array_object::vertex_array_object(gl::vertex_array_object&& src) noexcept
{
    *this = std::move(src);
}


gl::vertex_array_object& gl::vertex_array_object::operator=(vertex_array_object&& src) noexcept
{
    if (this != &src) {
        std::swap(m_gl_handler, src.m_gl_handler);
    }

    return *this;
}


gl::vertex_array_object::~vertex_array_object()
{
    glDeleteVertexArrays(1, &m_gl_handler);
}


void gl::vertex_array_object::bind() const
{
    glBindVertexArray(m_gl_handler);
}


void gl::vertex_array_object::unbind() const
{
    glBindVertexArray(0);
}
void gl::vertex_array_object::add_vertex_array(
    const gl::buffer<GL_ARRAY_BUFFER>& buf,
    uint32_t elements_count,
    uint32_t stride,
    uint32_t offset,
    uint32_t type,
    uint32_t normalized,
    int32_t location)
{
    if (location >= 0) {
        assert(location >= m_next_location_idx);
        m_next_location_idx = location;
    }

    bind_guard buf_guard(buf);
    bind_guard self_guard(*this);
    glEnableVertexAttribArray(m_next_location_idx);
    glVertexAttribPointer(m_next_location_idx, elements_count, type, normalized, stride, reinterpret_cast<void*>(offset));
    ++m_next_location_idx;
}
