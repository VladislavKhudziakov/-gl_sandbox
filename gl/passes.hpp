

#pragma once

#include <gl/bind_guard.hpp>
#include <gl/textures.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cinttypes>
#include <algorithm>
#include <array>
#include <optional>

namespace gl
{
  enum class attachment_type
  {
    color, depth
  };

  enum class depth_func
  {
    less, leq, eq,
  };

  enum class cull_func
  {
    front, back, off
  };

  template <attachment_type AttachmentType>
  class attachment
  {
  public:
    attachment() = default;
    ~attachment() = default;

    attachment(attachment&& src) {
      *this = std::move(src);
    }

    attachment&operator=(attachment&& src)
    {
      if (this != &src) {
        m_texture_handler = std::move(src.m_texture_handler);
      }

      return *this;
    }

    void resize(uint32_t w, uint32_t h)
    {
      m_texture_handler.fill<attachment<AttachmentType>>(nullptr, w, h);
    }

    texture<GL_TEXTURE_2D>& get_handler()
    {
      return m_texture_handler;
    }

    const texture<GL_TEXTURE_2D>& get_handler() const
    {
      return m_texture_handler;
    }

    operator uint32_t () const
    {
      return m_texture_handler;
    }

  private:
    texture<GL_TEXTURE_2D> m_texture_handler;
  };

  template<> struct texture_fill_resolver<attachment<attachment_type::color>, GL_TEXTURE_2D>
  {
    void operator()(void* data, int32_t w, int32_t h)
    {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }
  };

  template<> struct texture_fill_resolver<attachment<attachment_type::depth>, GL_TEXTURE_2D>
  {
    void operator()(void* data, int32_t w, int32_t h)
    {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }
  };


  class framebuffer
  {
  public:

    enum class attachment_target
    {
      color1, color2, color3, color4, depth
    };

    inline constexpr static uint32_t max_attachments = 4;

    framebuffer(uint32_t w, uint32_t h)
    : m_width(w)
    , m_height(h)
    {
      glGenFramebuffers(1, &m_gl_handler);
    }

    framebuffer(const framebuffer&) = delete;
    framebuffer&operator=(const framebuffer&) = delete;

    framebuffer(framebuffer&& src) noexcept
    {
      *this = std::move(src);
    }

    framebuffer&operator=(framebuffer&& src) noexcept
    {
      if (this != &src) {
        m_width = src.m_width;
        m_height = src.m_height;
        m_bound_target = src.m_bound_target;

        for (int i = 0; i < max_attachments; ++i) {
          if (src.m_color_attachments.at(i) != std::nullopt) {
            m_color_attachments[i].emplace(std::move(*src.m_color_attachments.at(i)));
          }
        }

        if (src.m_depth_attachment != std::nullopt) {
          m_depth_attachment.emplace(std::move(*src.m_depth_attachment));
        }

        std::swap(m_gl_handler, src.m_gl_handler);
      }

      return *this;
    }

    ~framebuffer()
    {
      glDeleteFramebuffers(1, &m_gl_handler);
    }

    void bind(int32_t bind_target) const
    {
      assert(bind_target == GL_DRAW_FRAMEBUFFER || bind_target == GL_READ_FRAMEBUFFER || bind_target == GL_FRAMEBUFFER);
      assert(glCheckFramebufferStatus(bind_target) == GL_FRAMEBUFFER_COMPLETE);
      glViewport(0, 0, m_width, m_height);
      glBindFramebuffer(bind_target, m_gl_handler);
      m_bound_target = bind_target;
    }

    void resize(uint32_t w, uint32_t h)
    {
      m_width = w;
      m_height = h;
      int32_t curr_binding;
      glGetIntegerv(GL_FRAMEBUFFER, &curr_binding);
      {
        bind_guard guard {*this, GL_FRAMEBUFFER};
        for (auto& attachment : m_color_attachments) {
          if (attachment == std::nullopt) {
            continue;
          }

          attachment->resize(w, h);
        }

        if (m_depth_attachment != std::nullopt) {
          m_depth_attachment->resize(w, h);
        }
      }
      glBindFramebuffer(GL_FRAMEBUFFER, curr_binding);
    }

    void unbind() const
    {
      glBindFramebuffer(m_bound_target, 0);
      m_bound_target = -1;
    }

    void blit(uint32_t dst_width, uint32_t dst_height)
    {
      int32_t read_fb, draw_fb;

      glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fb);
      glGetIntegerv(GL_DRAW_FRAMEBUFFER, &draw_fb);

      {
        bind_guard guard {*this, GL_READ_FRAMEBUFFER};
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        int32_t viewport[4];
        glGetIntegerv( GL_VIEWPORT, viewport );

        glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, dst_width, dst_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
      }

      glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fb);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fb);
    }

    template <typename DataType, attachment_target AttachmentTarget>
    void add_attachment()
    {
      bind_guard guard(*this, GL_DRAW_FRAMEBUFFER);

      if constexpr (AttachmentTarget == attachment_target::depth) {
        auto& attachment = get_attachment_optional<attachment_type::depth, AttachmentTarget>();
        assert(attachment == std::nullopt);
        attachment.emplace();
        attachment->resize(m_width, m_height);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *attachment, 0);
      } else {
        auto& attachment = get_attachment_optional<attachment_type::color, AttachmentTarget>();
        assert(attachment == std::nullopt);
        attachment.emplace();
        attachment->resize(m_width, m_height);
        const auto attachment_idx = uint32_t(AttachmentTarget);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment_idx, GL_TEXTURE_2D, *attachment, 0);
      }
    }

    template <attachment_target AttachmentTarget>
    auto& get_attachment()
    {
      if constexpr (AttachmentTarget == attachment_target::depth) {
        auto& a = get_attachment_optional<attachment_type::depth, AttachmentTarget>();
        assert(a != std::nullopt);
        return *a;
      } else {
        auto& a = get_attachment_optional<attachment_type::color, AttachmentTarget>();
        assert(a != std::nullopt);
        return *a;
      }
    }

    template <attachment_target AttachmentTarget>
    const auto& get_attachment() const
    {
      if constexpr (AttachmentTarget == attachment_target::depth) {
        auto& a = get_attachment_optional<attachment_type::depth, AttachmentTarget>();
        assert(a != std::nullopt);
        return *a;
      } else {
        auto& a = get_attachment_optional<attachment_type::color, AttachmentTarget>();
        assert(a != std::nullopt);
        return *a;
      }
    }

  private:
    template <attachment_type AttachmentType, attachment_target AttachmentTarget = attachment_target::color1>
    std::optional<attachment<AttachmentType>>& get_attachment_optional()
    {
      if constexpr (AttachmentType == attachment_type::depth) {
        return m_depth_attachment;
      } else {
        constexpr auto attachment_idx = uint32_t(AttachmentTarget);
        static_assert(attachment_idx < max_attachments);
        return m_color_attachments.at(attachment_idx);
      }
    }

    uint32_t m_gl_handler {0};
    mutable int32_t m_bound_target {-1};
    uint32_t m_width {0};
    uint32_t m_height {0};
    std::array<std::optional<attachment<attachment_type::color>>, max_attachments> m_color_attachments;
    std::optional<attachment<attachment_type::depth>> m_depth_attachment;
  };


  struct gpu_state
  {
    bool depth_test {false};
    depth_func depth_func = depth_func::leq;
    bool depth_write {false};
    bool color_write[4] {true, true, true, true};
    cull_func culling = cull_func::off;
    float clear_color[4] {0, 0, 0, 1};
    float clear_depth[4] {1, 1, 1, 1};
  };


  class pass
  {
  public:
    pass(gl::framebuffer fb)
    : m_framebuffer(std::move(fb))
    {

    }

    ~pass() = default;

    void bind() const
    {
      m_framebuffer.bind(GL_DRAW_FRAMEBUFFER);

      if (m_state.depth_test) {
        glEnable(GL_DEPTH_TEST);
        glClearBufferfv(GL_DEPTH, 0, m_state.clear_depth);
        glDepthMask(m_state.depth_write ? GL_TRUE : GL_FALSE);
        switch (m_state.depth_func) {
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
          m_state.color_write[0] ? GL_TRUE : GL_FALSE,
          m_state.color_write[1] ? GL_TRUE : GL_FALSE,
          m_state.color_write[2] ? GL_TRUE : GL_FALSE,
          m_state.color_write[3] ? GL_TRUE : GL_FALSE);

      for (int i = 0; i < framebuffer::max_attachments; ++i) {
        glClearBufferfv(GL_COLOR, i, m_state.clear_color);
      }

      switch (m_state.culling) {
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

    void unbind() const
    {
      m_framebuffer.unbind();
      reset();
    }

    void set_state(const gpu_state& new_state)
    {
      m_state = new_state;
    }

    framebuffer& get_framebuffer()
    {
      return m_framebuffer;
    }

    const framebuffer& get_framebuffer() const
    {
      return m_framebuffer;
    }

  private:
    void reset() const
    {
      glDisable(GL_DEPTH_TEST);
      glDepthMask(GL_TRUE);
      glDepthFunc(GL_LESS);
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glCullFace(GL_BACK);
      glDisable(GL_CULL_FACE);
    }

    framebuffer m_framebuffer;
    gpu_state m_state {};
  };
}
