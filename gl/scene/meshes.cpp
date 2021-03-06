

#include "meshes.hpp"


gl::scene::mesh::mesh(int32_t vsources_idx, indices_type it, uint32_t isz, uint32_t vsz)
    : m_vertices(vsources_idx)
    , m_indices_type(it)
    , m_indices_size(isz)
    , m_vertices_size(vsz)
{
}


int32_t gl::scene::mesh::get_vertices() const
{
    return m_vertices;
}


gl::scene::mesh::indices_type gl::scene::mesh::get_indices_type() const
{
    return m_indices_type;
}


uint32_t gl::scene::mesh::get_indices_size() const
{
    return m_indices_size;
}


uint32_t gl::scene::mesh::get_vertices_size() const
{
    return m_vertices_size;
}
