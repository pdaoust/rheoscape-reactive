#pragma once

#include <functional>
#include <cstdarg>
#include <string>
#include <vector>
#include <types/core_types.hpp>

namespace rheoscape {

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

  // Taken from https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf/49812018#49812018
  const std::string string_format(const char * const zc_format, ...) {
    // initialize use of the variable argument array
    va_list va_args;
    va_start(va_args, zc_format);

    // reliably acquire the size
    // from a copy of the variable argument array
    // and a functionally reliable call to mock the formatting
    va_list va_args_copy;
    va_copy(va_args_copy, va_args);
    const int i_len = std::vsnprintf(NULL, 0, zc_format, va_args_copy);
    va_end(va_args_copy);

    // return a formatted string without risking memory mismanagement
    // and without assuming any compiler or platform specific behavior
    std::vector<char> zc(i_len + 1);
    std::vsnprintf(zc.data(), zc.size(), zc_format, va_args);
    va_end(va_args);
    return std::string(zc.data(), i_len);
  }

}