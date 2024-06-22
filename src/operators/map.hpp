#ifndef RHEOSCAPE_MAP
#define RHEOSCAPE_MAP

#include <functional>
#include <core_types.hpp>

template <typename TIn, typename TOut>
source_fn<TOut> map_(source_fn<TIn> source, map_fn<TIn, TOut> mapper) {
  return [source, mapper](push_fn<TOut> push) {
    return source([mapper, push](TIn value) { push(mapper(value)); });
  };
}

template <typename TIn, typename TOut>
pipe_fn<TIn, TOut> map(map_fn<TIn, TOut> mapper) {
  return [mapper](source_fn<TIn> source) {
    return map_(source, mapper);
  };
}

#endif