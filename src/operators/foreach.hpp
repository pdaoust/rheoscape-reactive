#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  template <typename T>
  pullable_sink_fn<T> foreach(exec_fn<T> exec) {
    return [exec](source_fn<T> source) {
      return source([exec](T value) { exec(value); });
    };
  }

  template <typename T>
  pull_fn foreach(source_fn<T> source, exec_fn<T> exec) {
    return foreach(exec)(source);
  }

}