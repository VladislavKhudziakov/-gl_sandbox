
#pragma once

#include <gl/vertex_array_object.hpp>
#include <gltf/misc/data_storage.hpp>
#include <gltf/misc/element_utils.hpp>

#include <vector>

namespace gltf::utils
{
    template<typename DataType>
    void fill_vao(const std::vector<DataType>& data, gl::vertex_array_object& vao, int32_t loc)
    {
        gl::buffer<GL_ARRAY_BUFFER> buf;
        buf.fill(data.data(), data.size() * sizeof(DataType));
        vao.add_vertex_array(buf, data.front().length(), sizeof(DataType), 0, GL_FLOAT, GL_FALSE, loc);
    }

    inline void fill_vao(const gltf::data_storage& ds, gl::vertex_array_object& vao, int32_t loc)
    {
        gl::buffer<GL_ARRAY_BUFFER> buf;
        buf.fill(ds.data.data(), ds.data.size());

        const auto el_size = get_element_size(ds.c_type);
        const auto el_count = get_elements_count(ds.d_type);
        const auto stride = el_size * el_count;

        vao.add_vertex_array(buf, el_count, stride, 0, GLenum(ds.c_type), ds.normalized, loc);
    }
}

