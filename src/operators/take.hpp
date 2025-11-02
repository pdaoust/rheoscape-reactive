#pragma once

#include <functional>
#include <core_types.hpp>
#include <Endable.hpp>

namespace rheo::operators {

  // Re-emit a number of values from the source function,
  // then end the source.
  template <typename T>
  source_fn<Endable<T>> take(source_fn<T> source, size_t count) {
    return [source, count](push_fn<Endable<T>> push) {
      return source([count, push, i = 0](T&& value) mutable {
        if (i < count) {
          push(Endable<T>(std::forward<T>(value), i == count - 1));
          i ++;
        } else {
          push(Endable<T>());
        }
      });
    };
  }

  template <typename T>
  pipe_fn<Endable<T>, T> take(size_t count) {
    return [count](source_fn<T> source) {
      return take(source, count);
    };
  }

}