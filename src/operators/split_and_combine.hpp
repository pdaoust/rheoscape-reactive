#pragma once

#include <functional>
#include <types/core_types.hpp>
#include <operators/map.hpp>
#include <operators/share.hpp>
#include <operators/combine.hpp>
#include <operators/map.hpp>

namespace rheoscape::operators {

  template <typename TIn, typename TPipeIn1, typename TPipeOut1, typename TPipeIn2, typename TPipeOut2, typename CombineFn>
    requires concepts::Combiner2<CombineFn, TPipeOut1, TPipeOut2>
  auto split_and_combine(
    source_fn<TIn> source,
    map_fn<TPipeIn1, TIn> mapper1,
    pipe_fn<TPipeOut1, TPipeIn1> pipe1,
    map_fn<TPipeIn2, TIn> mapper2,
    pipe_fn<TPipeOut2, TPipeIn2> pipe2,
    CombineFn combiner
  ) {
    return combine(pipe1(map(source, mapper1)), pipe2(map(source, mapper2)))
      | map(combiner);
  }

  namespace detail {
    template <typename TIn, typename TPipeIn1, typename TPipeOut1, typename TPipeIn2, typename TPipeOut2, typename CombineFn>
    struct SplitAndCombinePipeFactory {
      map_fn<TPipeIn1, TIn> mapper1;
      pipe_fn<TPipeOut1, TPipeIn1> pipe1;
      map_fn<TPipeIn2, TIn> mapper2;
      pipe_fn<TPipeOut2, TPipeIn2> pipe2;
      CombineFn combiner;

      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return split_and_combine(source_fn<TIn>(std::move(source)), mapper1, pipe1, mapper2, pipe2, combiner);
      }
    };

    template <typename T, typename T1, typename T2, typename CombineFn>
    struct SplitAndCombineSimplePipeFactory {
      map_fn<T1, T> mapper1;
      pipe_fn<T1, T1> pipe1;
      map_fn<T2, T> mapper2;
      pipe_fn<T2, T2> pipe2;
      CombineFn combiner;

      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return split_and_combine<T, T1, T2>(source_fn<T>(std::move(source)), mapper1, pipe1, mapper2, pipe2, combiner);
      }
    };
  }

  template <typename TIn, typename TPipeIn1, typename TPipeOut1, typename TPipeIn2, typename TPipeOut2, typename CombineFn>
    requires concepts::Combiner2<CombineFn, TPipeOut1, TPipeOut2>
  auto split_and_combine(
    map_fn<TPipeIn1, TIn> mapper1,
    pipe_fn<TPipeOut1, TPipeIn1> pipe1,
    map_fn<TPipeIn2, TIn> mapper2,
    pipe_fn<TPipeOut2, TPipeIn2> pipe2,
    CombineFn combiner
  ) {
    return detail::SplitAndCombinePipeFactory<TIn, TPipeIn1, TPipeOut1, TPipeIn2, TPipeOut2, CombineFn>{
      mapper1, pipe1, mapper2, pipe2, combiner
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
  ) {
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
  ) {
    return detail::SplitAndCombineSimplePipeFactory<T, T1, T2, CombineFn>{
      mapper1, pipe1, mapper2, pipe2, combiner
    };
  }

}
