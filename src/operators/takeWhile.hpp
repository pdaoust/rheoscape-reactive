#ifndef RHEOSCAPE_TAKE_WHILE
#define RHEOSCAPE_TAKE_WHILE

#include <functional>
#include <core_types.hpp>

// Re-emit values from the source function until a condition is met,
// then end the source.
template <typename T>
source_fn<T> takeWhile(source_fn<T> source, filter_fn<T> condition) {
  return [source, condition](push_fn<T> push, end_fn end) {
    return source(
      [condition, push, end, running = true](T value) mutable {
        if (running) {
          if (condition(value)) {
            running = false;
            end();
          } else {
            push(value);
          }
        } else {
          end();
        }
      },
      end
    );
  };
}

template <typename T>
pipe_fn<T, T> takeWhile(filter_fn<T> condition) {
  return [condition](source_fn<T> source) {
    return takeWhile(source, condition);
  };
}

#endif