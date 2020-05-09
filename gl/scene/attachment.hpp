

#pragma once

#include <cinttypes>

namespace gl::scene
{
  class scene;

  enum class attachment_type
  {
    rgba8, rgba16f, depth24f
  };

  enum class pass_behaviour
  {
    clear, save
  };

  class attachment
  {
  public:
    explicit attachment(const scene& s, uint32_t, attachment_type);
    ~attachment() = default;

    void resize(uint32_t w, uint32_t h) const;
    uint32_t get_handler_idx() const;
    attachment_type get_type() const;
    void set_start_pass_behaviour(pass_behaviour);
    void set_end_pass_behaviour(pass_behaviour);
    void set_clear_values(const float* values);
    pass_behaviour get_start_pass_behaviour() const;
    pass_behaviour get_finish_pass_behaviour() const;
    const float* get_clear_values() const;

  private:
    const scene& m_scene;
    uint32_t m_texture_handler_idx;
    attachment_type m_type;
    pass_behaviour m_start_behaviour = pass_behaviour::clear;
    pass_behaviour m_finish_behaviour = pass_behaviour::save;
    float m_clear_values[4] {1, 1, 1, 1};
  };
}

