

#include "shaders.hpp"

gl::program::program(
    const gl::shader<GL_VERTEX_SHADER>& vs,
    const gl::shader<GL_FRAGMENT_SHADER>& fs,
    const gl::shader<GL_GEOMETRY_SHADER>& gs)
    : program(vs, fs)
{
    glAttachShader(m_gl_handler, gs.m_gl_handler);
}


gl::program::program(const gl::shader<GL_VERTEX_SHADER>& vs, const gl::shader<GL_FRAGMENT_SHADER>& fs)
    : m_gl_handler{glCreateProgram()}
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


gl::program::~program()
{
    glDeleteProgram(m_gl_handler);
}


gl::program::program(gl::program&& src) noexcept
{
    *this = std::move(src);
}


gl::program& gl::program::operator=(gl::program&& src) noexcept
{
    if (this != &src) {
        std::swap(m_gl_handler, src.m_gl_handler);
    }

    return *this;
}


void gl::program::bind() const
{
    glUseProgram(m_gl_handler);
}


void gl::program::unbind() const
{
    glUseProgram(0);
}
