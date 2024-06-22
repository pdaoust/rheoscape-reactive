#ifndef RHEOSCAPE_START_WHEN
#define RHEOSCAPE_START_WHEN

#include <functional>
#include <core_types.hpp>

template <typename T>
source_fn<T> startWhen_(source_fn<T> source, filter_fn<T> condition) {
  return [source, condition](push_fn<T> push) {
    bool started = false;
    return source([condition, push, &started](T value) {
      if (!started && condition(value)) {
        started = true;
      }
      if (started) {
        push(value);
      }
    });
  };
}

template <typename T>
pipe_fn<T, T> startWhen(filter_fn<T> condition) {
  return [condition](source_fn<T> source) {
    return startWhen_(source, condition);
  };
}

#endif