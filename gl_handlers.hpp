

#pragma once

#include <va_for_each.hpp>

#include <glad/glad.h>
#include <glm/matrix.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <cinttypes>

namespace gl
{

  template <typename T>
  class bind_guard
  {
  public:
    template <typename ...Args>
    explicit bind_guard(const T& ref, Args&& ...args)
      : m_ref {ref}
    {
      ref.bind(std::forward<Args>(args)...);
    }

    ~bind_guard()
    {
      m_ref.unbind();
    }

  private:
    const T& m_ref;
  };

  template <uint32_t GlBufferType>
  class buffer
  {
  public:
    enum class usage_type {
      static_usage, dynamic_usage
    };

    buffer()
    {
      glGenBuffers(1, &m_gl_handler);
    }

    buffer(const buffer&) = delete;
    buffer&operator=(const buffer&) = delete;

    buffer(buffer&& src) noexcept
    {
      *this = std::move(src);
    }

    buffer&operator=(buffer&& src) noexcept
    {
      if (this != &src) {
        std::swap(m_gl_handler, src.m_gl_handler);
      }

      return *this;
    }

    ~buffer()
    {
      glDeleteBuffers(1, &m_gl_handler);
    }

    void bind() const
    {
      glBindBuffer(GlBufferType, m_gl_handler);
    }

    void unbind() const
    {
      glBindBuffer(GlBufferType, 0);
    }

    void fill(void* data, uint32_t data_size, usage_type type = usage_type::static_usage)
    {
      bind_guard guard(*this);
      uint32_t usage = type == usage_type::static_usage ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
      glBufferData(GlBufferType, data_size, data, GL_STATIC_DRAW);
    }

  private:
    uint32_t m_gl_handler {0};
  };

  class vertex_array_object
  {
  public:
    vertex_array_object()
    {
      glGenVertexArrays(1, &m_gl_handler);
    }

    vertex_array_object(const vertex_array_object&) = delete;
    vertex_array_object&operator=(const vertex_array_object&) = delete;

    vertex_array_object(vertex_array_object&& src) noexcept
    {
      *this = std::move(src);
    }

    vertex_array_object&operator=(vertex_array_object&& src) noexcept
    {
      if (this != &src) {
        std::swap(m_gl_handler, src.m_gl_handler);
      }

      return *this;
    }

    ~vertex_array_object()
    {
      glDeleteVertexArrays(1, &m_gl_handler);
    }

    template <typename DataType, uint32_t BufferType>
    void add_vertex_array(
        const buffer<BufferType>& buf,
        uint32_t elements_count,
        uint32_t stride = sizeof(DataType),
        uint32_t offset = 0,
        uint32_t type = GL_FLOAT,
        uint32_t normalized = GL_FALSE)
    {
      assert(BufferType == GL_ARRAY_BUFFER && "invalid buffer type.");
      bind_guard buf_guard(buf);
      bind_guard self_guard(*this);
      glEnableVertexAttribArray(m_next_location_idx);
      glVertexAttribPointer(m_next_location_idx, elements_count, type, normalized, stride, reinterpret_cast<void*>(offset));
      ++m_next_location_idx;
    }

    void bind() const
    {
      glBindVertexArray(m_gl_handler);
    }

    void unbind() const
    {
      glBindVertexArray(0);
    }

  private:
    uint32_t m_gl_handler {0};
    uint32_t m_next_location_idx {0};
  };

  template <uint32_t GlShaderType>
  class shader
  {
    friend class program;
  public:
    explicit shader(const std::string& source)
      : m_gl_handler{glCreateShader(GlShaderType)}
    {
      auto source_ptr = source.c_str();
      glShaderSource(m_gl_handler, 1, &source_ptr, nullptr);
      glCompileShader(m_gl_handler);

      int32_t success;
      glGetShaderiv(m_gl_handler, GL_COMPILE_STATUS, &success);

      if (success == GL_FALSE) {
        int32_t log_length;
        glGetShaderiv(m_gl_handler, GL_INFO_LOG_LENGTH, &log_length);
        auto log = std::make_unique<char[]>(log_length);
        glGetShaderInfoLog(m_gl_handler, log_length, nullptr, log.get());
        glDeleteShader(m_gl_handler);
        throw std::runtime_error(std::string(log.get()));
      }
    }

    shader(const shader&) = delete;
    shader&operator=(const shader&) = delete;

    shader(shader&& src) noexcept
    {
      *this = std::move(src);
    }

    shader&operator=(shader&& src) noexcept
    {
      if (this != &src) {
        std::swap(m_gl_handler, src.m_gl_handler);
      }

      return *this;
    }

    ~shader()
    {
      glDeleteShader(m_gl_handler);
    }

  private:
    operator uint32_t() {
      return m_gl_handler;
    }
    uint32_t m_gl_handler {0};
  };


  class program
  {
  public:
//    template <typename ...ShaderType>
//    explicit program(ShaderType&& ...shaders)
//      : m_gl_handler {glCreateProgram()}
//    {
//      utils::va_for_each(std::forward<ShaderType>(shaders)..., [this](uint32_t shader) {
//        glAttachShader(m_gl_handler, shader);
//      });
//
//      glLinkProgram(m_gl_handler);
//
//      int32_t success;
//      glGetProgramiv(m_gl_handler, GL_LINK_STATUS, &success);
//
//      if (success == GL_FALSE) {
//        int32_t log_len;
//        glGetProgramiv(m_gl_handler, GL_INFO_LOG_LENGTH, &log_len);
//        auto log = std::make_unique<char[]>(log_len);
//        glDeleteProgram(m_gl_handler);
//        throw std::runtime_error(log.get());
//      }
//    }

    program(const shader<GL_VERTEX_SHADER>& vs, const shader<GL_FRAGMENT_SHADER>& fs)
        : m_gl_handler {glCreateProgram()}
    {
      glAttachShader(m_gl_handler, vs.m_gl_handler);
      glAttachShader(m_gl_handler, fs.m_gl_handler);

      glLinkProgram(m_gl_handler);

      int32_t success;
      glGetProgramiv(m_gl_handler, GL_LINK_STATUS, &success);

      if (success == GL_FALSE) {
        int32_t log_len;
        glGetProgramiv(m_gl_handler, GL_INFO_LOG_LENGTH, &log_len);
        auto log = std::make_unique<char[]>(log_len);
        glGetProgramInfoLog(m_gl_handler, log_len, nullptr, log.get());
        glDeleteProgram(m_gl_handler);
        throw std::runtime_error(log.get());
      }
    }

    ~program()
    {
      glDeleteProgram(m_gl_handler);
    }

    program(const program&) = delete;
    program&operator=(const program&) = delete;

    program(program&& src) noexcept
    {
      *this = std::move(src);
    }

    program&operator=(program&& src) noexcept
    {
      if (this != &src) {
        std::swap(m_gl_handler, src.m_gl_handler);
      }

      return *this;
    }

    void bind() const
    {
      glUseProgram(m_gl_handler);
    }

    void unbind() const
    {
      glUseProgram(0);
    }

    template <typename UniformType, typename ...Args>
    void set_uniform(const std::string& uniform_name, const UniformType& uniform_data, Args&& ...args)
    {
      auto loc = glGetUniformLocation(m_gl_handler, uniform_name.c_str());

      if (loc >= 0) {
        uniform_resolver<UniformType>{}(loc, uniform_data, std::forward<Args>(args)...);
      }
    }

  private:
    uint32_t m_gl_handler {0};

    template <typename T>
    class uniform_resolver
    {
    };

    template<> class uniform_resolver<glm::mat4>
    {
    public:
      void operator()(uint32_t location, const glm::mat4& matrix)
      {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
      }
    };

    template<> class uniform_resolver<glm::mat4*>
    {
    public:
      void operator()(uint32_t location, const glm::mat4* matrices, size_t size)
      {
        glUniformMatrix4fv(location, size, GL_FALSE, glm::value_ptr(*matrices));
      }
    };

    template<> class uniform_resolver<glm::vec3>
    {
    public:
      void operator()(uint32_t location, const glm::vec3& v)
      {
        glUniform3fv(location, 1, glm::value_ptr(v));
      }
    };

    template<> class uniform_resolver<int32_t>
    {
    public:
      void operator()(uint32_t location, int32_t data)
      {
        glUniform1i(location, data);
      }
    };
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

  private:
    uint32_t m_gl_handler {0};

    template <typename DataType, uint32_t FillTextureType>
    struct texture_fill_resolver
    {
    };

    template<> struct texture_fill_resolver<float, GL_TEXTURE_2D>
    {
      void operator()(void* data, int32_t w, int32_t h, bool rgb = false, bool gen_mips = false)
      {
        glTexImage2D(GL_TEXTURE_2D, 0, rgb ? GL_RGB32F : GL_RGBA32F, w, h, 0, rgb ? GL_RGB32F : GL_RGBA32F, GL_FLOAT, data);

        if (gen_mips) {
          glGenerateMipmap(GL_TEXTURE_2D);
        }
      }
    };
  };
}
