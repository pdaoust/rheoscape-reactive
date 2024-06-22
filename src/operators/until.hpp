#ifndef RHEOSCAPE_UNTIL
#define RHEOSCAPE_UNTIL

#include <functional>
#include <core_types.hpp>

template <typename T>
source_fn<T> until_(source_fn<T> source, filter_fn<T> condition) {
  return [source, condition](push_fn<T> push) {
    bool running = true;
    return source([condition, push, &running](T value) {
      if (running && condition(value)) {
        running = false;
      }
      if (running) {
        push(value);
      }
    });
  };
}

template <typename T>
pipe_fn<T, T> until(filter_fn<T> condition) {
  return [condition](source_fn<T> source) {
    return until_(source, condition);
  };
}

#endif