#ifndef RHEOSCAPE_MAP
#define RHEOSCAPE_MAP

#include <functional>
#include <core_types.hpp>

template <typename TIn, typename TOut>
pipe_fn<TIn, TOut> map(std::function<TOut(TIn)> mapper) {
  return [mapper](source_fn<TIn> source) {
    return [mapper, source](push_fn<TOut> push) {
      return source([mapper, push](TIn value) { push(mapper(value)); });
    };
  };
}

#endif