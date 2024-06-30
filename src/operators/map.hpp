#ifndef RHEOSCAPE_MAP
#define RHEOSCAPE_MAP

#include <functional>
#include <core_types.hpp>

template <typename TOut, typename TIn>
source_fn<TOut> map(source_fn<TIn> source, map_fn<TOut, TIn> mapper) {
  return [source, mapper](push_fn<TOut> push) {
    return source([mapper, push](TIn value) { push(mapper(value)); });
  };
}

template <typename TOut, typename TIn>
pipe_fn<TOut, TIn> map(map_fn<TOut, TIn> mapper) {
  return [mapper](source_fn<TIn> source) {
    return map(source, mapper);
  };
}

#endif