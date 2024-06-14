#ifndef RHEOSCAPE_TAP
#define RHEOSCAPE_TAP

#include <functional>
#include <core_types.hpp>

template <typename T>
pipe_fn<T, T> tap(std::function<void(T)> exec) {
  return [exec](source_fn<T> source) {
    source([exec](T value) { exec(value); });
    return source;
  };
}

#endif