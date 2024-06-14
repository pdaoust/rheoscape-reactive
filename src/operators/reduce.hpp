#ifndef RHEOSCAPE_REDUCE
#define RHEOSCAPE_REDUCE

#include <functional>
#include <core_types.hpp>

template <typename T>
pipe_fn<T, T> reduce(std::function<T(T, T)> reducer) {
  std::optional<T> acc;
  return [reducer, &acc](source_fn<T> source) {
    return [reducer, source, &acc](push_fn<T> push) {
      return source([reducer, push, &acc](T value) {
        if (!acc.has_value()) {
          acc = value;
          push(value);
        } else {
          acc = reducer(acc.value(), value);
          push(acc.value());
        }
      });
    };
  };
}

#endif