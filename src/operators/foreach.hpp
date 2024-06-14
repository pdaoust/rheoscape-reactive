#ifndef RHEOSCAPE_FOREACH
#define RHEOSCAPE_FOREACH

#include <functional>
#include <core_types.hpp>

template <typename T>
cap_fn<T> foreach(std::function<void(T)> exec) {
  return [exec](source_fn<T> source) {
    source([exec](T value) { exec(value); });
  };
}

#endif