#pragma once

#include <core_types.hpp>

namespace rheo::sources {

  // A source function that ends immediately.
  template <typename T>
  source_fn<Endable<T>> done() {
    return [isDone = false](push_fn<Endable<T>> push) mutable{
      if (!isDone) {
        push(Endable<T>());
        isDone = true;
      }
    };
  }

}