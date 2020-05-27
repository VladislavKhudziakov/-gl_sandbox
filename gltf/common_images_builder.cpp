

#include "common_images_builder.hpp"

#include <third/tinygltf/tiny_gltf.h>
#include <third/tinygltf/stb_image.h>


void gltf::common_images_builder::make_image(gl::scene::scene& scene, const tinygltf::Image& img)
{
    gl::texture<GL_TEXTURE_2D> tex;
    if (!img.image.empty()) {
        tex.fill(img.image.data(), img.width, img.height, img.component);
    } else {
        assert(!img.uri.empty());

        int32_t w, h, c;

        std::unique_ptr<uint8_t, std::function<void(uint8_t*)>> image_data(
            stbi_load(img.uri.c_str(), &w, &h, &c, 0),
            [](uint8_t* data) {
                stbi_image_free(data);
            });

        tex.fill(static_cast<const uint8_t*>(image_data.get()), w, h, c);
    }

    scene.textures.emplace_back(std::move(tex));
}
