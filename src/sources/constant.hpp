#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::sources {

  template <typename T>
  source_fn<T> constant(T value) {
    return [value](push_fn<T> push) {
      return [push, value](){ push(value); };
    };
  }

}