

#pragma once

#include <gl/bind_guard.hpp>
#include <gl/textures.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cinttypes>
#include <algorithm>
#include <array>
#include <optional>
#include <variant>
#include <vector>

namespace gl
{
  enum class attachment_type
  {
    color_rgba8u, color_rgba16f, depth_24f
  };

  enum class depth_func
  {
    less, leq, eq,
  };

  enum class cull_func
  {
    front, back, off
  };

  enum class blend_func
  {
    alpha, add, multiply, off
  };

  struct gpu_state
  {
    bool color_write[4] {true, true, true, true};
    depth_func depth_func = depth_func::leq;
    bool depth_test {false};
    bool depth_write {false};
    cull_func culling = cull_func::off;
    blend_func blend = blend_func::off;
  };

  enum class pass_behaviour
  {
    clear, save
  };

  class attachment
  {
  public:
    explicit attachment(attachment_type type);
    ~attachment() = default;

    attachment(attachment&& src) noexcept;

    attachment&operator=(attachment&& src) noexcept;

    void resize(uint32_t w, uint32_t h);

    texture<GL_TEXTURE_2D>& get_handler();

    const texture<GL_TEXTURE_2D>& get_handler() const;

    operator uint32_t () const;

    attachment_type get_type() const;

    void set_start_pass_behaviour(pass_behaviour);
    void set_end_pass_behaviour(pass_behaviour);
    void set_clear_values(float values[4]);
    pass_behaviour get_start_pass_behaviour() const;
    pass_behaviour get_finish_pass_behaviour() const;
    const float* get_clear_values() const;
  private:
    texture<GL_TEXTURE_2D> m_texture_handler;
    attachment_type m_type;
    pass_behaviour m_start_behaviour = pass_behaviour::clear;
    pass_behaviour m_finish_behaviour = pass_behaviour::save;
    float m_clear_values[4] {1, 1, 1, 1};
  };

  template<> struct texture_fill_resolver<attachment, GL_TEXTURE_2D>
  {
    void operator()(void* data, int32_t w, int32_t h, attachment_type type)
    {
      switch (type) {

      case attachment_type::color_rgba8u:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        break;
      case attachment_type::color_rgba16f:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
        break;
      case attachment_type::depth_24f:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        break;
      }
    }
  };

  class framebuffer
  {
  public:

    enum class attachment_target
    {
      color1, color2, color3, color4, depth
    };

    inline constexpr static uint32_t max_attachments = 5;

    framebuffer(uint32_t w, uint32_t h);

    framebuffer(const framebuffer&) = delete;
    framebuffer&operator=(const framebuffer&) = delete;

    framebuffer(framebuffer&& src) noexcept;

    framebuffer&operator=(framebuffer&& src) noexcept;

    ~framebuffer();

    void bind(int32_t bind_target) const;

    void resize(uint32_t w, uint32_t h);

    void unbind() const;

    void blit(uint32_t dst_width, uint32_t dst_height);
    void blit(const framebuffer& dst);

    template <attachment_target AttachmentTarget, attachment_type AttachmentType>
    void add_attachment()
    {
      bind_guard guard(*this, GL_DRAW_FRAMEBUFFER);
      auto& attachment = get_attachment_optional<AttachmentTarget>();
      assert(attachment == std::nullopt);
      attachment.emplace(AttachmentType);
      attachment->resize(m_width, m_height);

      if constexpr (AttachmentTarget == attachment_target::depth) {
        static_assert(AttachmentType == attachment_type::depth_24f);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *attachment, 0);
      } else {
        static_assert(AttachmentType == attachment_type::color_rgba8u || AttachmentType == attachment_type::color_rgba16f);
        auto attachment_idx = uint32_t(AttachmentTarget);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment_idx, GL_TEXTURE_2D, *(attachment), 0);
        auto drawbufs = get_drawbuffers();
        glDrawBuffers(drawbufs.size(), drawbufs.data());
      }
    }

    template <attachment_target AttachmentTarget>
    auto& get_attachment()
    {
      auto& a = get_attachment_optional<AttachmentTarget>();
      assert(a != std::nullopt);
      return *a;
    }

    template <attachment_target AttachmentTarget>
    const auto& get_attachment() const
    {
        auto& a = get_attachment_optional<AttachmentTarget>();
        assert(a != std::nullopt);
        return *a;
    }

    std::pair<uint32_t, uint32_t> get_screen_size() const;

  private:
    template <attachment_target AttachmentTarget>
    auto& get_attachment_optional()
    {
        constexpr auto attachment_idx = uint32_t(AttachmentTarget);
        static_assert(attachment_idx < max_attachments);
        return m_attachments.at(attachment_idx);
    }

    template <typename Callable>
    void clear(Callable&& f) const
    {
      for (size_t i = 0; i < m_attachments.size(); ++i) {
        const auto& attachment = m_attachments[i];
        if (attachment != std::nullopt && f(*attachment) == gl::pass_behaviour::clear) {
          switch (attachment->get_type()) {
          case attachment_type::color_rgba8u:
            [[fallthrough]];
          case attachment_type::color_rgba16f:
            glClearBufferfv(GL_COLOR, i, attachment->get_clear_values());
            break;
          case attachment_type::depth_24f:
            glClearBufferfv(GL_DEPTH, 0, attachment->get_clear_values());
            break;
          }
        }
      }
    }

    std::vector<GLenum> get_drawbuffers();

    static void blit(
        uint32_t from_handler, uint32_t dst_handler, uint32_t from_width, uint32_t from_height, uint32_t dst_width, uint32_t dst_height);

    uint32_t m_gl_handler {0};
    mutable int32_t m_bound_target {-1};
    uint32_t m_width {0};
    uint32_t m_height {0};
    std::array<std::optional<attachment>, max_attachments> m_attachments;
  };


  class pass
  {
  public:
    explicit pass(gl::framebuffer fb);
    pass(pass&&) = default;
    pass& operator=(pass&&) = default;

    ~pass() = default;

    void bind() const;

    void unbind() const;

    void set_state(const gpu_state& new_state) const;

    framebuffer& get_framebuffer();

    const framebuffer& get_framebuffer() const;

  private:
    void reset() const;

    framebuffer m_framebuffer;
  };
}
