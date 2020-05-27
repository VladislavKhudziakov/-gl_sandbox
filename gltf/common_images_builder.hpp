


#pragma once

#include <gltf/images_builder.hpp>

namespace gltf
{
    class common_images_builder : public images_builder
    {
    public:
        void make_image(gl::scene::scene& scene, const tinygltf::Image& img) override;
    };
}

