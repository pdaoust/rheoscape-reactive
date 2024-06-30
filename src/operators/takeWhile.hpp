#ifndef RHEOSCAPE_UNTIL
#define RHEOSCAPE_UNTIL

#include <functional>
#include <core_types.hpp>

// Re-emit values from the source function until a condition is met,
// then end the source.
template <typename T>
source_fn<T> takeWhile(source_fn<T> source, filter_fn<T> condition) {
  return [source, condition](push_fn<T> push, end_fn end) {
    bool running = true;
    return source([condition, push, end, running](T value) mutable {
      if (running && condition(value)) {
        running = false;
        end();
      }
      if (running) {
        push(value);
      }
    });
  };
}

template <typename T>
pipe_fn<T, T> takeWhile(filter_fn<T> condition) {
  return [condition](source_fn<T> source) {
    return until(source, condition);
  };
}

#endif