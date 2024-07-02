#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo {

  // Re-emit values from the source function until a condition is met,
  // then end the source.
  template <typename T>
  source_fn<T> take(source_fn<T> source, size_t count) {
    return [source, count](push_fn<T> push, end_fn end) {
      return source(
        [count, push, end, i = 0](T value) mutable {
          if (i < count) {
            push(value);
            i ++;
          } else {
            end();
          }
        },
        end
      );
    };
  }

  template <typename T>
  pipe_fn<T, T> take(filter_fn<T> condition) {
    return [condition](source_fn<T> source) {
      return take(source, condition);
    };
  }

}