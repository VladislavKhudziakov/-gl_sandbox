

#pragma once

#include <cinttypes>
#include <array>

#include <gl/scene/attachment.hpp>

namespace gl::scene
{
    class scene;
    class attachment;

    enum class framebuffer_target
    {
        read,
        draw
    };

    class framebuffer
    {
    public:
        enum attachment_target
        {
            color1,
            color2,
            color3,
            color4,
            depth,
            max_attachments
        };

        framebuffer(scene& s, uint32_t i, uint32_t w, uint32_t h);
        ~framebuffer() = default;

        void blit(uint32_t dst_width, uint32_t dst_height);
        void blit(const framebuffer& dst);
        void bind(framebuffer_target target = framebuffer_target::draw) const;
        void unbind() const;
        void resize(uint32_t w, uint32_t h);

        void add_attachment(attachment_target, int32_t attachment_idx);
        void remove_attachment(attachment_target);
        int32_t get_attachment(attachment_target) const;
        std::pair<uint32_t, uint32_t> get_size() const;

    private:
        void clear(const std::function<gl::scene::pass_behaviour(const attachment&)>& f) const;
        scene& m_scene;
        uint32_t m_handler_idx;
        uint32_t m_width;
        uint32_t m_height;
        std::array<int32_t, max_attachments> m_attachments{-1, -1, -1, -1, -1};
    };
} // namespace gl::scene
