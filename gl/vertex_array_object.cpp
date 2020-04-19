

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
