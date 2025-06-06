#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  template <typename T>
  source_fn<T> tap(source_fn<T> source, exec_fn<T> exec) {
    return [source, exec](push_fn<T> push, end_fn end) {
      return source(
        [exec, push](T value) {
          exec(value);
          push(value);
        },
        end
      );
    };
  }

  template <typename T>
  pipe_fn<T, T> tap(exec_fn<T> exec) {
    return [exec](source_fn<T> source) {
      return tap(source, exec);
    };
  }

}