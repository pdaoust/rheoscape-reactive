#pragma once

#include <type_traits>
#include <types/core_types.hpp>

namespace rheoscape {

  // A wrapper that annotates an arbitrary pipe callable
  // with explicit input_type and output_type aliases,
  // making it satisfy the Pipe concept
  // without requiring the underlying callable to declare those aliases itself.
  //
  // This is useful for wrapping pipe factories like settle()
  // whose pipe factory structs don't declare input_type/output_type,
  // or for any situation where you need to carry
  // the value types alongside the pipe.
  template <typename TIn, typename TOut, typename PipeFn>
  struct typed_pipe_wrapper {
    using input_type = TIn;
    using output_type = TOut;

    PipeFn pipe;

    template <typename SourceT>
      requires concepts::SourceOf<SourceT, TIn>
    RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
      return pipe(std::move(source));
    }
  };

  // Factory function to create a typed_pipe_wrapper.
  // For same-type pipes (most common case), only one type parameter is needed.
  //
  // Usage:
  //   auto pipe = typed_pipe<int>(settle(clock, duration));
  //   auto pipe = typed_pipe<int, float>(some_transforming_pipe);
  template <typename TIn, typename TOut = TIn, typename PipeFn>
  auto typed_pipe(PipeFn pipe) {
    return typed_pipe_wrapper<TIn, TOut, PipeFn>{std::move(pipe)};
  }

}
