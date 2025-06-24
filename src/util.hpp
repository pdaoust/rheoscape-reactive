#pragma once

#include <functional>
#include <cstdarg>
#include <string>
#include <vector>
#include <core_types.hpp>

namespace rheo {

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
  const std::string string_format(const char * const zcFormat, ...) {
    // initialize use of the variable argument array
    va_list vaArgs;
    va_start(vaArgs, zcFormat);

    // reliably acquire the size
    // from a copy of the variable argument array
    // and a functionally reliable call to mock the formatting
    va_list vaArgsCopy;
    va_copy(vaArgsCopy, vaArgs);
    const int iLen = std::vsnprintf(NULL, 0, zcFormat, vaArgsCopy);
    va_end(vaArgsCopy);

    // return a formatted string without risking memory mismanagement
    // and without assuming any compiler or platform specific behavior
    std::vector<char> zc(iLen + 1);
    std::vsnprintf(zc.data(), zc.size(), zcFormat, vaArgs);
    va_end(vaArgs);
    return std::string(zc.data(), iLen);
  }

  // A class to help with the ending of a
  // source function that take multiple upstream source functions
  // and ends when any of them end.
  struct EndAny {
    const end_fn downstream_end_fn;
    bool ended;
    const end_fn upstream_end_fn;

    EndAny(end_fn downstream_end_fn)
    :
      downstream_end_fn(downstream_end_fn),
      ended(false),
      upstream_end_fn([this]() {
        this->ended = true;
        this->downstream_end_fn();
      })
    { }
  };

}