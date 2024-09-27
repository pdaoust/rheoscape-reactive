#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo {

  // There is no source_fn factory because foreach doesn't produce a source_fn.
  // And foreaching something is literally just binding a push function to it.
  // The function below is just there for use in `Pipe`.

  template <typename T>
  cap_fn<T> foreach(exec_fn<T> exec) {
    return [exec](source_fn<T> source) {
      source(
        [exec](T value) { exec(value); },
        [](){}
      );
    };
  }

}