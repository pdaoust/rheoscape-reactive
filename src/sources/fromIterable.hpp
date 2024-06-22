#ifndef RHEOSCAPE_FROM_ITERATOR
#define RHEOSCAPE_FROM_ITERATOR

#include <array>
#include <functional>
#include <iterator>
#include <core_types.hpp>

template <typename TVal, template <typename...> typename TIter>
source_fn<TVal> fromIterable(TIter<TVal> iterable) {
  return [iterable](push_fn<TVal> push) {
    auto i = iterable.begin();
    return [iterable, push, &i]() {
      if (i < iterable.end()) {
        push(*i);
        i ++;
      }
    };
  };
}

#endif