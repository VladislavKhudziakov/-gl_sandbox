

#include "passes.hpp"


gl::framebuffer::framebuffer(uint32_t w, uint32_t h)
: m_width(w)
, m_height(h)
{
  glGenFramebuffers(1, &m_gl_handler);
}


gl::framebuffer::framebuffer(gl::framebuffer &&src) noexcept
{
  *this = std::move(src);
}


gl::framebuffer &gl::framebuffer::operator=(gl::framebuffer &&src) noexcept
{
  if (this != &src) {
    m_width = src.m_width;
    m_height = src.m_height;
    m_bound_target = src.m_bound_target;

    for (int i = 0; i < max_attachments; ++i) {
      if (src.m_attachments.at(i) != std::nullopt) {
        m_attachments[i].emplace(std::move(*src.m_attachments.at(i)));
      }
    }

    std::swap(m_gl_handler, src.m_gl_handler);
  }

  return *this;
}


gl::framebuffer::~framebuffer()
{
  glDeleteFramebuffers(1, &m_gl_handler);
}


void gl::framebuffer::bind(int32_t bind_target) const
{
  assert(bind_target == GL_DRAW_FRAMEBUFFER || bind_target == GL_READ_FRAMEBUFFER || bind_target == GL_FRAMEBUFFER);
  assert(glCheckFramebufferStatus(bind_target) == GL_FRAMEBUFFER_COMPLETE);
  glViewport(0, 0, m_width, m_height);
  glBindFramebuffer(bind_target, m_gl_handler);
  m_bound_target = bind_target;
  clear([](const attachment& a) {return a.get_start_pass_behaviour(); });
}


void gl::framebuffer::resize(uint32_t w, uint32_t h)
{
  m_width = w;
  m_height = h;
  int32_t curr_binding;
  glGetIntegerv(GL_FRAMEBUFFER, &curr_binding);
  {
    bind_guard guard {*this, GL_FRAMEBUFFER};
    for (auto& attachment : m_attachments) {

      if (attachment == std::nullopt) {
        continue;
      }

      attachment->resize(w, h);
    }
  }
  glBindFramebuffer(GL_FRAMEBUFFER, curr_binding);
}


void gl::framebuffer::unbind() const
{
  clear([](const attachment& a) {return a.get_finish_pass_behaviour(); });
  glBindFramebuffer(m_bound_target, 0);
  m_bound_target = -1;
}


void gl::framebuffer::blit(
    uint32_t from_handler, uint32_t dst_handler,
    uint32_t from_width, uint32_t from_height,
    uint32_t dst_width, uint32_t dst_height)
{
  int32_t read_fb, draw_fb;

  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fb);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fb);

  {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, from_handler);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_handler);
    glBlitFramebuffer(0, 0, from_width, from_height, 0, 0, dst_width, dst_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
  }

  glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fb);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fb);
}


void gl::framebuffer::blit(uint32_t dst_width, uint32_t dst_height)
{
  blit(m_gl_handler, 0, m_width, m_height, dst_width, dst_height);
}


void gl::framebuffer::blit(const gl::framebuffer &dst)
{
  blit(m_gl_handler, dst.m_gl_handler, m_width, m_height, dst.m_width, dst.m_height);
}


std::vector<GLenum> gl::framebuffer::get_drawbuffers()
{
  std::vector<GLenum> res;
  for (int i = 0; i < max_attachments - 1; ++i) {
    const auto& curr_attachment = m_attachments[i];
    if (curr_attachment == std::nullopt || curr_attachment->get_type() == attachment_type::depth_24f) {
      continue;
    }
    res.emplace_back(GL_COLOR_ATTACHMENT0 + i);
  }

  return res;
}


std::pair<uint32_t, uint32_t> gl::framebuffer::get_screen_size() const
{
  return {m_width, m_height};
}


gl::attachment::attachment(gl::attachment_type type)
  : m_type(type)
{
}


gl::attachment::attachment(gl::attachment &&src) noexcept
{
  *this = std::move(src);
}


gl::attachment &gl::attachment::operator=(gl::attachment &&src) noexcept
{
  if (this != &src) {
    m_texture_handler = std::move(src.m_texture_handler);
    m_type = src.m_type;
  }

  return *this;
}


void gl::attachment::resize(uint32_t w, uint32_t h)
{
  m_texture_handler.fill<attachment>(nullptr, w, h, m_type);
}


gl::texture<GL_TEXTURE_2D> &gl::attachment::get_handler()
{
  return m_texture_handler;
}


const gl::texture<0x0DE1> &gl::attachment::get_handler() const
{
  return m_texture_handler;
}


gl::attachment::operator uint32_t() const
{
    return m_texture_handler;
}


gl::attachment_type gl::attachment::get_type() const
{
  return m_type;
}


void gl::attachment::set_start_pass_behaviour(gl::pass_behaviour b)
{
  m_start_behaviour = b;
}


void gl::attachment::set_end_pass_behaviour(gl::pass_behaviour b)
{
  m_finish_behaviour = b;
}
void gl::attachment::set_clear_values(float values[4])
{
  m_clear_values[0] = values[0];
  m_clear_values[1] = values[1];
  m_clear_values[2] = values[2];
  m_clear_values[3] = values[3];
}


gl::pass_behaviour gl::attachment::get_start_pass_behaviour() const
{
  return m_start_behaviour;
}


gl::pass_behaviour gl::attachment::get_finish_pass_behaviour() const
{
  return m_finish_behaviour;
}


const float *gl::attachment::get_clear_values() const
{
  return m_clear_values;
}


gl::pass::pass(gl::framebuffer fb)
    : m_framebuffer(std::move(fb))
{
}


void gl::pass::bind() const
{
  m_framebuffer.bind(GL_DRAW_FRAMEBUFFER);
}


void gl::pass::unbind() const
{
  m_framebuffer.unbind();
  reset();
}


void gl::pass::set_state(const gl::gpu_state& new_state) const
{
  if (new_state.depth_test) {
    glEnable(GL_DEPTH_TEST);
    glDepthMask(new_state.depth_write ? GL_TRUE : GL_FALSE);
    switch (new_state.depth_func) {
    case depth_func::less:
      glDepthFunc(GL_LESS);
      break;
    case depth_func::leq:
      glDepthFunc(GL_LEQUAL);
      break;
    case depth_func::eq:
      glDepthFunc(GL_EQUAL);
      break;
    }
  }

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


gl::framebuffer &gl::pass::get_framebuffer()
{
  return m_framebuffer;
}


const gl::framebuffer &gl::pass::get_framebuffer() const
{
  return m_framebuffer;
}


void gl::pass::reset() const
{
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glCullFace(GL_BACK);
  glDisable(GL_CULL_FACE);
}
