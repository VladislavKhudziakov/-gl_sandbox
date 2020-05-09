

#include "framebuffer.hpp"

#include <gl/scene/scene.hpp>



gl::scene::framebuffer::framebuffer(
    gl::scene::scene &s, uint32_t i, uint32_t w, uint32_t h)
: m_scene(s)
, m_handler_idx(i)
, m_width(w)
, m_height(h)
{
}


void gl::scene::framebuffer::blit(uint32_t dst_width, uint32_t dst_height)
{
  m_scene.fbos.at(m_handler_idx).blit(0, m_width, m_height, dst_width, dst_height);
}


void gl::scene::framebuffer::blit(const gl::scene::framebuffer &dst)
{
  const auto& dst_handler = m_scene.fbos.at(dst.m_handler_idx);
  m_scene.fbos.at(m_handler_idx).blit(dst_handler, m_width, m_height, dst.m_width, dst.m_height);
}


void gl::scene::framebuffer::bind(gl::scene::framebuffer_target target) const
{
  GLenum gl_target = target == gl::scene::framebuffer_target::read ? GL_READ_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER;
  m_scene.fbos.at(m_handler_idx).bind(gl_target);
  glViewport(0, 0, m_width, m_height);
  clear([](const attachment& a) {return a.get_start_pass_behaviour();});
}


void gl::scene::framebuffer::unbind() const
{
  m_scene.fbos.at(m_handler_idx).unbind();
  clear([](const attachment& a) {return a.get_finish_pass_behaviour(); });
}


void gl::scene::framebuffer::add_attachment(gl::scene::framebuffer::attachment_target target, int32_t idx)
{
  assert(target != max_attachments);

  const auto& curr_attachment = m_scene.attachments.at(idx);

  const auto& attachment_texture = std::get<gl::texture<GL_TEXTURE_2D>>(m_scene.textures.at(curr_attachment.get_handler_idx()));

  int32_t draw_fb;

  curr_attachment.resize(m_width, m_height);

  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fb);

  {
    bind_guard g(*this);

    m_attachments.at(target) = idx;

    if (target == depth) {
      assert(curr_attachment.get_type() == attachment_type::depth24f);
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, attachment_texture, 0);
    } else {
      assert(curr_attachment.get_type() != attachment_type::depth24f);
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + target, GL_TEXTURE_2D, attachment_texture, 0);
      std::vector<GLenum> drawbufs;

      for (size_t i = 0; i < max_attachments - 1; ++i) {
        if (m_attachments.at(i) >= 0) {
          drawbufs.emplace_back(GL_COLOR_ATTACHMENT0 + i);
        }
      }

      glDrawBuffers(drawbufs.size(), drawbufs.data());
    }
  }

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fb);
}


void gl::scene::framebuffer::remove_attachment(gl::scene::framebuffer::attachment_target target)
{
  m_attachments.at(target) = -1;
}


int32_t gl::scene::framebuffer::get_attachment(
    gl::scene::framebuffer::attachment_target target) const
{
  return m_attachments.at(target);
}


std::pair<uint32_t, uint32_t> gl::scene::framebuffer::get_size() const
{
  return {m_width, m_height};
}


void gl::scene::framebuffer::resize(uint32_t w, uint32_t h)
{
  m_width = w;
  m_height = h;

  for (const auto attachment : m_attachments) {
    if (attachment >= 0) {
      m_scene.attachments.at(attachment).resize(w, h);
    }
  }
}

void gl::scene::framebuffer::clear(const std::function<gl::scene::pass_behaviour(const attachment &)> &f) const
{
  for (size_t i = 0; i < gl::scene::framebuffer::max_attachments; ++i) {
    const auto attachment_idx = m_attachments.at(i);

    if (attachment_idx < 0) {
      continue;
    }

    const auto& attachment = m_scene.attachments.at(attachment_idx);

    if (f(attachment) == gl::scene::pass_behaviour::clear) {
      switch (attachment.get_type()) {
      case attachment_type::rgba8:
        [[fallthrough]];
      case attachment_type::rgba16f:
        glClearBufferfv(GL_COLOR, i, attachment.get_clear_values());
        break;
      case attachment_type::depth24f:
        glClearBufferfv(GL_DEPTH, 0, attachment.get_clear_values());
        break;
      }
    }
  }
}
