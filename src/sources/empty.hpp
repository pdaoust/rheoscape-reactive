#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo {

  template <typename T>
  source_fn<T> empty() {
    return [](push_fn<T> _1, end_fn _2) {
      return [](){};
    };
  }

}