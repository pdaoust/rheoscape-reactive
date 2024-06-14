#ifndef RHEOSCAPE_MERGE
#define RHEOSCAPE_MERGE

#include <functional>
#include <core_types.hpp>

// Don't use this with two pulls, or a push and a pull!
// In fact, it won't even let you pull.
template <typename T>
pipe_fn<T, T> merge(source_fn<T> other) {
  return [other](source_fn<T> source) {
    return [other, source](push_fn<TOut> push) {
      source([push](TIn value) { push(value); });
      other([push](TIn value) { push(value); });
      return [](){};
    };
  };
}

#endif