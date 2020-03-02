

#pragma once

#include <algorithm>
#include <functional>

namespace utils
{
  template <typename ArgumentType, typename ...ArgsType>
  void va_for_each(ArgumentType&& argument, ArgsType&& ...args, const std::function<void()>& cb)
  {
    cb(std::forward<ArgumentType>(argument));

    if (sizeof...(args) == 0) {
      return;
    }

    va_for_each(std::forward<ArgsType>(args)..., cb);
  }
}
