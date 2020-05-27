

#pragma once

#include <gl/buffer.hpp>
#include <glad/glad.h>

#include <cinttypes>
#include <algorithm>

namespace gl
{
    class vertex_array_object
    {
    public:
        vertex_array_object();

        vertex_array_object(const vertex_array_object&) = delete;
        vertex_array_object& operator=(const vertex_array_object&) = delete;

        vertex_array_object(vertex_array_object&& src) noexcept;

        vertex_array_object& operator=(vertex_array_object&& src) noexcept;

        ~vertex_array_object();

        void add_vertex_array(
            const buffer<GL_ARRAY_BUFFER>& buf,
            uint32_t elements_count,
            uint32_t stride,
            uint32_t offset = 0,
            uint32_t type = GL_FLOAT,
            uint32_t normalized = GL_FALSE,
            int32_t location = -1);

        void bind() const;
        void unbind() const;

    private:
        uint32_t m_gl_handler{0};
        uint32_t m_next_location_idx{0};
    };
} // namespace gl
