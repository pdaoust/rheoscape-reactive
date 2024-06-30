#ifndef RHEOSCAPE_CONSTANT
#define RHEOSCAPE_CONSTANT

#include <functional>
#include <core_types.hpp>

template <typename T>
source_fn<T> constant(T value) {
  return [value](push_fn<T> push, end_fn _) {
    return [push, value](){ push(value); };
  };
}

#endif