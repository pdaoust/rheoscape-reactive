#ifndef RHEOSCAPE_TAP
#define RHEOSCAPE_TAP

#include <functional>
#include <core_types.hpp>

template <typename T>
source_fn<T> tap_(source_fn<T> source, exec_fn<T> exec) {
  source([exec](T value) { exec(value); });
  return source;
}

template <typename T>
pipe_fn<T, T> tap(exec_fn<T> exec) {
  return [exec](source_fn<T> source) {
    return tap_(source, exec);
  };
}

#endif