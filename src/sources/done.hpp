#pragma once

#include <core_types.hpp>

namespace rheo::sources {

  // A source function that ends immediately.

  template <typename T>
  pull_fn done(push_fn<T> _, end_fn end) {
    end();
    return [end]() { end(); };
  }
}