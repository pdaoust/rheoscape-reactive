#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/map.hpp>
#include <operators/zip.hpp>

namespace rheo::operators {
  
  template <typename TIn, typename TPipeIn1, typename TPipeOut1, typename TPipeIn2, typename TPipeOut2, typename TOut>
  source_fn<TOut> splitAndZip(
    source_fn<TIn> source,
    map_fn<TPipeIn1, TIn> mapper1,
    pipe_fn<TPipeOut1, TPipeIn1> pipe1,
    map_fn<TPipeIn2, TIn> mapper2,
    pipe_fn<TPipeOut2, TPipeIn2> pipe2,
    combine2_fn<TOut, TPipeIn1, TPipeIn2> combiner
  ) {
    return zip(
      pipe1(map(source, mapper1)),
      pipe2(map(source, mapper2)),
      combiner
    );
  }

  template <typename TIn, typename TPipeIn1, typename TPipeOut1, typename TPipeIn2, typename TPipeOut2, typename TOut>
  source_fn<TOut> splitAndZip(
    map_fn<TPipeIn1, TIn> mapper1,
    pipe_fn<TPipeOut1, TPipeIn1> pipe1,
    map_fn<TPipeIn2, TIn> mapper2,
    pipe_fn<TPipeOut2, TPipeIn2> pipe2,
    combine2_fn<TOut, TPipeIn1, TPipeIn2> combiner
  ) {
    return [mapper1, pipe1, mapper2, pipe2, combiner](source_fn<TIn> source) {
      return splitAndZip(
        source,
        mapper1,
        pipe1,
        mapper2,
        pipe2,
        combiner
      );
    };
  }

  template <typename T, typename T1, typename T2>
  source_fn<T> splitAndZip(
    source_fn<T> source,
    map_fn<T1, T> mapper1,
    pipe_fn<T1, T1> pipe1,
    map_fn<T2, T> mapper2,
    pipe_fn<T2, T2> pipe2,
    combine2_fn<T, T1, T2> combiner
  ) {
    return splitAndZip<T, T1, T1, T2, T2, T>(source, mapper1, pipe1, mapper2, pipe2, combiner);
  }

  template <typename T, typename T1, typename T2>
  pipe_fn<T, T> splitAndZip(
    map_fn<T1, T> mapper1,
    pipe_fn<T1, T1> pipe1,
    map_fn<T2, T> mapper2,
    pipe_fn<T2, T2> pipe2,
    combine2_fn<T, T1, T2> combiner
  ) {
    return [mapper1, pipe1, mapper2, pipe2, combiner](source_fn<T> source) {
      return splitAndZip<T, T1, T2>(source, mapper1, pipe1, mapper2, pipe2, combiner);
    };
  }

}