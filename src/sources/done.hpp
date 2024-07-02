#pragma once

#include <core_types.hpp>

namespace rheo {

  // A source function that ends immediately.
  template <typename T>
  source_fn<T> done() {
    return [](push_fn<T> _, end_fn endFn) {
      return [endFn]() { endFn(); };
    };
  }

}