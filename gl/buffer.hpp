

#pragma once

#include <gl/bind_guard.hpp>

#include <glad/glad.h>
#include <cinttypes>
#include <algorithm>


namespace gl
{
    template<uint32_t GlBufferType>
    class buffer
    {
    public:
        enum class usage_type
        {
            static_usage,
            dynamic_usage
        };

        buffer()
        {
            glGenBuffers(1, &m_gl_handler);
        }

        buffer(const buffer&) = delete;
        buffer& operator=(const buffer&) = delete;

        buffer(buffer&& src) noexcept
        {
            *this = std::move(src);
        }

        buffer& operator=(buffer&& src) noexcept
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

        void fill(const void* data, uint32_t data_size, usage_type type = usage_type::static_usage)
        {
            bind_guard guard(*this);
            uint32_t usage = type == usage_type::static_usage ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
            glBufferData(GlBufferType, data_size, data, usage);
        }

    private:
        uint32_t m_gl_handler{0};
    };
} // namespace gl
