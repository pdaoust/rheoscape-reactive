#ifndef RHEOSCAPE_COUNT
#define RHEOSCAPE_COUNT

#include <functional>
#include <core_types.hpp>

template <typename T>
source_fn<size_t> count(source_fn<T> source) {
  return [source](push_fn<size_t> push, end_fn end) {
    size_t i = 0;
    return source(
      [i, push](T value) mutable {
        i ++;
        push(i);
      },
      end
    );
  };
}

#endif