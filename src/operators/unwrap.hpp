#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/filterMap.hpp>

namespace rheo::operators {
  
  template <typename T>
  source_fn<T> unwrap(source_fn<std::optional<T>> source) {
    return [source](push_fn<T> push) {
      return source([push](std::optional<T> value) {
        if (value.has_value()) {
          push(value.value());
        }
      });
    };
  }

  template <typename T>
  pipe_fn<T, T> unwrap() {
    return [](source_fn<T> source) {
      return unwrap(source);
    };
  }

}