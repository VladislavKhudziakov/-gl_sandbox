


#pragma once

#include <gl/scene/scene.hpp>

namespace tinygltf
{
    class Image;
}

namespace gltf
{
    class images_builder
    {
    public:
        virtual ~images_builder() = default;
        virtual void make_image(gl::scene::scene&, const tinygltf::Image& img) = 0;
    };
}

