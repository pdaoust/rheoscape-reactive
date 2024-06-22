#ifndef RHEOSCAPE_MERGE
#define RHEOSCAPE_MERGE

#include <functional>
#include <core_types.hpp>

// Don't use this with two pulls, or a push and a pull!
// In fact, it won't even let you pull.

template <typename T>
source_fn<T> merge(source_fn<T> source1, source_fn<T> source2) {
  return [source1, source2](push_fn<T> push) {
    source1([push](T value) { push(value); });
    source2([push](T value) { push(value); });
    return [](){};
  };
}

template <typename T>
pipe_fn<T, T> merge(source_fn<T> source2) {
  return [source2](source_fn<T> source1) {
    return merge(source1, source2);
  };
}

#endif