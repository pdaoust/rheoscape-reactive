#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo {

  template <typename T>
  source_fn<T> constant(T value) {
    return [value](push_fn<T> push, end_fn _) {
      return [push, value](){ push(value); };
    };
  }

}