#ifndef RHEOSCAPE_FILTER
#define RHEOSCAPE_FILTER

#include <functional>
#include <core_types.hpp>

template <typename T>
pipe_fn<T, T> filter(std::function<bool(T)> filterer) {
  return [filterer](source_fn<T> source) {
    return [filterer, source](push_fn<T> push) {
      return source([filterer, push](T value) { if (filterer(value)) push(value); });
    };
  };
}

#endif