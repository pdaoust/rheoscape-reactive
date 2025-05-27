#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  // There is no source_fn factory because foreach doesn't produce a source_fn.
  // And foreaching something is literally just binding a push function to it.
  // The function below is just there for use in `Pipe`.

  template <typename T>
  pullable_sink_fn<T> foreach(exec_fn<T> exec) {
    return [exec](source_fn<T> source) {
      return source(
        [exec](T value) { exec(value); },
        [](){}
      );
    };
  }

}