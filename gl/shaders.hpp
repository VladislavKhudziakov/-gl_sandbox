

#pragma once

#include <glad/glad.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <memory>

namespace gl {
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

template<> class uniform_resolver<float>
{
public:
  void operator()(uint32_t location, float v)
  {
    glUniform1fv(location, 1, &v);
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
    void operator()(uint32_t location, const glm::vec3& v, size_t count = 1)
    {
      glUniform3fv(location, count, glm::value_ptr(v));
    }
  };

  template<> class uniform_resolver<glm::vec2>
  {
  public:
    void operator()(uint32_t location, const glm::vec2& v, size_t count = 1)
    {
      glUniform2fv(location, count, glm::value_ptr(v));
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

  template<> class uniform_resolver<double>
  {
  public:
    void operator()(uint32_t location, double data)
    {
      glUniform1f(location, data);
    }
  };

  template <uint32_t GlShaderType> class shader {
    friend class program;

  public:
    explicit shader(const std::string &source)
        : m_gl_handler{glCreateShader(GlShaderType)} {
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

    shader(const shader &) = delete;
    shader &operator=(const shader &) = delete;

    shader(shader &&src) noexcept { *this = std::move(src); }

    shader &operator=(shader &&src) noexcept {
      if (this != &src) {
        std::swap(m_gl_handler, src.m_gl_handler);
      }

      return *this;
    }

    ~shader() { glDeleteShader(m_gl_handler); }

  private:
    operator uint32_t() { return m_gl_handler; }
    uint32_t m_gl_handler{0};
  };

  class program {
  public:
    program(const shader<GL_VERTEX_SHADER> &vs, const shader<GL_FRAGMENT_SHADER>& fs);
    program(const shader<GL_VERTEX_SHADER> &vs, const shader<GL_FRAGMENT_SHADER>& fs,  const shader<GL_GEOMETRY_SHADER>& gs);
    ~program();

    program(const program &) = delete;
    program &operator=(const program &) = delete;

    program(program &&src) noexcept;

    program &operator=(program &&src) noexcept;

    void bind() const;
    void unbind() const;

    template <typename UniformType, typename... Args>
    void set_uniform(const std::string &uniform_name, const UniformType &uniform_data, Args&&... args) const
  {
      auto loc = glGetUniformLocation(m_gl_handler, uniform_name.c_str());

      if (loc >= 0) {
        uniform_resolver<UniformType>{}(loc, uniform_data, std::forward<Args>(args)...);
      }
    }

  private:
    uint32_t m_gl_handler{0};
  };
}
