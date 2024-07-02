#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo {
  
  template <typename T>
  source_fn<T> filter(source_fn<T> source, filter_fn<T> filterer) {
    return [source, filterer](push_fn<T> push, end_fn end) {
      return source(
        [filterer, push](T value) { if (filterer(value)) push(value); },
        end
      );
    };
  }

  template <typename T>
  pipe_fn<T, T> filter(filter_fn<T> filterer) {
    return [filterer](source_fn<T> source) {
      return filter(source, filterer);
    };
  }

  // This is a filter_fn<std::optional<T>>.
  template <typename T>
  bool notEmpty(std::optional<T> value) {
    return value.has_value();
  }

}