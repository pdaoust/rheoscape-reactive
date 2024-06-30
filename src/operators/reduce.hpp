#ifndef RHEOSCAPE_REDUCE
#define RHEOSCAPE_REDUCE

#include <functional>
#include <core_types.hpp>

template <typename T>
source_fn<T> reduce_(source_fn<T> source, reduce_fn<T> reducer) {
  return [source, reducer](push_fn<T> push) {
    std::optional<T> acc;
    return source([reducer, acc, push](T value) mutable {
      if (!acc.has_value()) {
        acc.emplace(value);
        push(value);
      } else {
        acc.emplace(reducer(acc.value(), value));
        push(acc.value());
      }
    });
  };
}

template <typename T>
pipe_fn<T, T> reduce(reduce_fn<T> reducer) {
  return [reducer](source_fn<T> source) {
    return reduce_(source, reducer);
  };
}

#endif