#ifndef RHEOSCAPE_COUNT
#define RHEOSCAPE_COUNT

#include <functional>
#include <core_types.hpp>

template <typename TIn, typename TCount>
source_fn<TCount> count(source_fn<TIn> source) {
  return [source](push_fn<TCount> push) {
    TCount count = 0;
    return source([&count, push](TIn value) {
      count ++;
      push(count);
    });
  };
}

#endif