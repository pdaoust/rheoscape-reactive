#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/map.hpp>
#include <operators/share.hpp>
#include <operators/combine.hpp>
#include <operators/map_tuple.hpp>

namespace rheo::operators {

  template <typename TIn, typename TPipeIn1, typename TPipeOut1, typename TPipeIn2, typename TPipeOut2, typename CombineFn>
    requires concepts::Combiner2<CombineFn, TPipeOut1, TPipeOut2>
  auto split_and_combine(
    source_fn<TIn> source,
    map_fn<TPipeIn1, TIn> mapper1,
    pipe_fn<TPipeOut1, TPipeIn1> pipe1,
    map_fn<TPipeIn2, TIn> mapper2,
    pipe_fn<TPipeOut2, TPipeIn2> pipe2,
    CombineFn combiner
  ) -> source_fn<std::invoke_result_t<CombineFn, TPipeOut1, TPipeOut2>> {
    return combine(pipe1(map(source, mapper1)), pipe2(map(source, mapper2)))
      | map_tuple(combiner);
  }

  template <typename TIn, typename TPipeIn1, typename TPipeOut1, typename TPipeIn2, typename TPipeOut2, typename CombineFn>
    requires concepts::Combiner2<CombineFn, TPipeOut1, TPipeOut2>
  auto split_and_combine(
    map_fn<TPipeIn1, TIn> mapper1,
    pipe_fn<TPipeOut1, TPipeIn1> pipe1,
    map_fn<TPipeIn2, TIn> mapper2,
    pipe_fn<TPipeOut2, TPipeIn2> pipe2,
    CombineFn combiner
  ) -> pipe_fn<std::invoke_result_t<CombineFn, TPipeOut1, TPipeOut2>, TIn> {
    return [mapper1, pipe1, mapper2, pipe2, combiner](source_fn<TIn> source) {
      return split_and_combine(
        source,
        mapper1,
        pipe1,
        mapper2,
        pipe2,
        combiner
      );
    };
  }

  template <typename T, typename T1, typename T2, typename CombineFn>
    requires concepts::Combiner2<CombineFn, T1, T2>
  auto split_and_combine(
    source_fn<T> source,
    map_fn<T1, T> mapper1,
    pipe_fn<T1, T1> pipe1,
    map_fn<T2, T> mapper2,
    pipe_fn<T2, T2> pipe2,
    CombineFn combiner
  ) -> source_fn<std::invoke_result_t<CombineFn, T1, T2>> {
    return split_and_combine<T, T1, T1, T2, T2>(source, mapper1, pipe1, mapper2, pipe2, combiner);
  }

  template <typename T, typename T1, typename T2, typename CombineFn>
    requires concepts::Combiner2<CombineFn, T1, T2>
  auto split_and_combine(
    map_fn<T1, T> mapper1,
    pipe_fn<T1, T1> pipe1,
    map_fn<T2, T> mapper2,
    pipe_fn<T2, T2> pipe2,
    CombineFn combiner
  ) -> pipe_fn<std::invoke_result_t<CombineFn, T1, T2>, T> {
    return [mapper1, pipe1, mapper2, pipe2, combiner](source_fn<T> source) {
      return split_and_combine<T, T1, T2>(source, mapper1, pipe1, mapper2, pipe2, combiner);
    };
  }

}