

#pragma once

#include <algorithm>

namespace gl
{
  template <typename T>
  class bind_guard
  {
  public:
    template <typename ...Args>
    explicit bind_guard(const T& ref, Args&& ...args)
        : m_ref {ref}
    {
      ref.bind(std::forward<Args>(args)...);
    }

    ~bind_guard()
    {
      m_ref.unbind();
    }

  private:
    const T& m_ref;
  };
}