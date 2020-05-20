

#pragma once

#include <unordered_map>

#include <gl/scene/material.hpp>
#include <gl_handlers.hpp>

namespace gl::scene
{
    class mesh
    {
    public:
        enum class indices_type
        {
            none,
            u8 = GL_UNSIGNED_BYTE,
            u16 = GL_UNSIGNED_SHORT,
            u32 = GL_UNSIGNED_INT,
        };

        mesh(int32_t vsources_idx, indices_type ind_type, uint32_t ind_sz, uint32_t vert_sz);
        mesh(mesh&&) = default;
        mesh& operator=(mesh&&) = default;
        ~mesh() = default;
        int32_t get_vertices() const;
        indices_type get_indices_type() const;
        uint32_t get_indices_size() const;
        uint32_t get_vertices_size() const;

    private:
        int32_t m_vertices;
        indices_type m_indices_type;
        uint32_t m_indices_size;
        uint32_t m_vertices_size;
    };
} // namespace gl::scene
