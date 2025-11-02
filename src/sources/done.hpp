#pragma once

#include <core_types.hpp>
#include <Endable.hpp>

namespace rheo::sources {

  // A source function that ends immediately.
  template <typename T>
  source_fn<Endable<T>> done() {
    return [](push_fn<Endable<T>> push) {
      return [push, isDone = false]() mutable {
        if (!isDone) {
          push(Endable<T>());
          isDone = true;
        }
      };
    };
  }

}