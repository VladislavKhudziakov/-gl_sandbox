


#pragma once

#include <gl/vertex_array_object.hpp>
#include <gl/shaders.hpp>

namespace gltf::primitives
{
    gl::vertex_array_object make_cube();
    gl::program make_cubemap_shader();
}
