

#pragma once

#include <gl/bind_guard.hpp>

#include <glad/glad.h>
#include <cinttypes>

namespace gl
{
  template <typename DataType, uint32_t FillTextureType>
  struct texture_fill_resolver
  {
  };

  template<> struct texture_fill_resolver<float, GL_TEXTURE_2D>
  {
    void operator()(void* data, int32_t w, int32_t h, uint32_t channels = 4, bool is_f16 = true, bool gen_mips = false)
    {
      int32_t texture_storage_type = -1;
      int32_t texture_data_type = -1;

      if (channels != 4) {
        glTexParameteri(GL_TEXTURE_2D, GL_UNPACK_ALIGNMENT, 1);
      }

      switch (channels) {
        case 1:
          texture_storage_type = is_f16 ? GL_R16F : GL_R32F;
          texture_data_type = GL_R32F;
          break;
        case 2:
          texture_storage_type = is_f16 ? GL_RG16F : GL_RG32F;
          texture_data_type = GL_RG32F;
          break;
        case 3:
          texture_storage_type = is_f16 ? GL_RGB16F : GL_RGB32F;
          texture_data_type = GL_RGB32F;
          break;
        case 4:
          texture_storage_type = is_f16 ? GL_RGBA16F : GL_RGBA32F;
          texture_data_type = GL_RGBA32F;
          break;
        default:
          throw std::runtime_error("invalid channels count");
      }

      glTexImage2D(GL_TEXTURE_2D, 0, texture_storage_type, w, h, 0, texture_data_type, GL_FLOAT, data);

      if (channels != 4) {
        glTexParameteri(GL_TEXTURE_2D, GL_UNPACK_ALIGNMENT, 4);
      }

      if (gen_mips) {
        glGenerateMipmap(GL_TEXTURE_2D);
      }
    }
  };

  template<> struct texture_fill_resolver<uint8_t, GL_TEXTURE_2D>
  {
    void operator()(void* data, int32_t w, int32_t h, bool rgb = false, bool gen_mips = false)
    {
      if (rgb) {
        glTexParameteri(GL_TEXTURE_2D, GL_UNPACK_ALIGNMENT, 1);
      }

      glTexImage2D(GL_TEXTURE_2D, 0, rgb ? GL_RGB8 : GL_RGBA8, w, h, 0, rgb ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, data);

      if (gen_mips) {
        glGenerateMipmap(GL_TEXTURE_2D);
      }

      if (rgb) {
        glTexParameteri(GL_TEXTURE_2D, GL_UNPACK_ALIGNMENT, 4);
      }
    }
  };

  template <uint32_t TextureType>
  class texture
  {
  public:
    explicit texture(int32_t filter_type = GL_NEAREST, int32_t wrap = GL_CLAMP_TO_EDGE)
    {
      glGenTextures(1, &m_gl_handler);

      bind_guard guard(*this);

      glTexParameteri(TextureType, GL_TEXTURE_MIN_FILTER, filter_type);
      glTexParameteri(TextureType, GL_TEXTURE_MAG_FILTER, filter_type);

      glTexParameteri(TextureType, GL_TEXTURE_WRAP_S, wrap);
      glTexParameteri(TextureType, GL_TEXTURE_WRAP_T, wrap);

      if constexpr (TextureType == GL_TEXTURE_CUBE_MAP) {
        glTexParameteri(TextureType, GL_TEXTURE_WRAP_R, wrap);
      }

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    }

    texture(const texture&) = delete;
    texture&operator=(const texture&) = delete;

    texture(texture&& src) noexcept
    {
      *this = std::move(src);
    }

    texture&operator=(texture&& src) noexcept
    {
      if (this != &src) {
        std::swap(m_gl_handler, src.m_gl_handler);
      }

      return *this;
    }

    ~texture()
    {
      glDeleteTextures(1, &m_gl_handler);
    }

    void bind(int32_t texture_target = 0) const
    {
      glActiveTexture(GL_TEXTURE0 + texture_target);
      glBindTexture(TextureType, m_gl_handler);
    }

    void unbind() const
    {
      glBindTexture(TextureType, m_gl_handler);
    }

    template <typename DataType, typename ...Args>
    void fill(void* data, int32_t w, int32_t h, Args&& ...args)
    {
      bind_guard guard(*this);
      texture_fill_resolver<DataType, TextureType>{}(data, w, h, std::forward<Args>(args)...);
    }

    operator uint32_t() const
    {
      return m_gl_handler;
    }

  private:
    uint32_t m_gl_handler {0};
  };
}
