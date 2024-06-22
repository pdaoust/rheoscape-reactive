#ifndef RHEOSCAPE_FILTER
#define RHEOSCAPE_FILTER

#include <functional>
#include <core_types.hpp>

template <typename T>
source_fn<T> filter_(source_fn<T> source, filter_fn<T> filterer) {
  return [source, filterer](push_fn<T> push) {
    return source([filterer, push](T value) { if (filterer(value)) push(value); });
  };
}

template <typename T>
pipe_fn<T, T> filter(filter_fn<T> filterer) {
  return [filterer](source_fn<T> source) {
    return filter_(source, filterer);
  };
}

// This is a filter_fn<std::optional<T>>.
template <typename T>
bool notEmpty(std::optional<T> value) {
  return value.has_value();
}

#endif