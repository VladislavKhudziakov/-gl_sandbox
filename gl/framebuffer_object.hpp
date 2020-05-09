

#pragma once

#include <gl/textures.hpp>

namespace gl
{
  class framebuffer_object
  {
  public:
    framebuffer_object();
    ~framebuffer_object();

    framebuffer_object(const framebuffer_object&) = delete;
    framebuffer_object&operator=(const framebuffer_object&) = delete;

    framebuffer_object(framebuffer_object&& src) noexcept;
    framebuffer_object&operator=(framebuffer_object&& src) noexcept;

    void bind(GLenum);
    void unbind();

    operator uint32_t() const;

    void blit(uint32_t dst_handler, uint32_t from_width, uint32_t from_height, uint32_t dst_width, uint32_t dst_height);
  private:
    uint32_t m_gl_handler;
    GLenum m_bound_target = GLenum(-1);
  };
}

