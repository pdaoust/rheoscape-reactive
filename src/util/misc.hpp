#pragma once

#include <functional>
#include <cstdarg>
#include <string>
#include <vector>

namespace rheoscape::util {

  // Convert a function pointer to a std::function.
  template <typename Func>
  auto fp2sf(Func ptr) {
    return std::function<std::remove_pointer_t<Func>>(ptr);
  }

  // Convert a lambda expression into a std::function.
  template <typename Lambda>
  auto le2sf(Lambda le) {
    return (std::function<decltype(le)>)le;
  }

}