

#include "pass.hpp"

#include <gl/scene/scene.hpp>


gl::scene::pass::pass(const gl::scene::scene& s, uint32_t i)
: m_scene(s)
, m_framebuffer_idx(i)
{
}


void gl::scene::pass::bind() const
{
  m_scene.framebuffers.at(m_framebuffer_idx).bind();
  assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}


void gl::scene::pass::unbind() const
{
  reset();
  m_scene.framebuffers.at(m_framebuffer_idx).unbind();
}


void gl::scene::pass::set_state(const gl::scene::gpu_state &new_state) const
{
  switch (new_state.depth_func) {
    case depth_func::off:
      glDisable(GL_DEPTH_TEST);
      break;
    case depth_func::less:
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LESS);
      break;
    case depth_func::leq:
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);
      break;
    case depth_func::eq:
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_EQUAL);
      break;
  }

  glDepthMask(new_state.depth_write ? GL_TRUE : GL_FALSE);

  glColorMask(
      new_state.color_write[0] ? GL_TRUE : GL_FALSE,
      new_state.color_write[1] ? GL_TRUE : GL_FALSE,
      new_state.color_write[2] ? GL_TRUE : GL_FALSE,
      new_state.color_write[3] ? GL_TRUE : GL_FALSE);

  switch (new_state.culling) {
  case cull_func::front:
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    break;
  case cull_func::back:
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    break;
  case cull_func::off:
    glDisable(GL_CULL_FACE);
    break;
  }
}


uint32_t gl::scene::pass::get_framebuffer_idx() const
{
  return m_framebuffer_idx;
}


void gl::scene::pass::reset() const
{
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glCullFace(GL_BACK);
  glDisable(GL_CULL_FACE);
}
