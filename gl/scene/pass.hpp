

#pragma once

#include <cinttypes>

namespace gl::scene
{
  class scene;

  enum class depth_func
  {
    less, leq, eq, off
  };

  enum class cull_func
  {
    front, back, off
  };

  enum class blend_func
  {
    alpha, add, multiply, off
  };

  struct gpu_state
  {
    bool color_write[4] {true, true, true, true};
    bool depth_write {false};

    depth_func depth_func = depth_func::off;
    cull_func culling = cull_func::off;
    blend_func blend = blend_func::off;
  };

  class pass
  {
  public:
    pass(const scene&, uint32_t);
    ~pass() = default;
    void bind() const;
    void unbind() const;
    void set_state(const gpu_state& new_state) const;
    uint32_t get_framebuffer_idx() const;

  private:
    void reset() const;
  private:
    const scene& m_scene;
    uint32_t m_framebuffer_idx;
  };
}
