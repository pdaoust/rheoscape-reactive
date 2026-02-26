#pragma once

#include <tuple>
#include <type_traits>
#include <types/core_types.hpp>

namespace rheoscape::util {

  namespace detail {
    template <size_t I, typename SourceT, typename PipeTuple>
    auto apply_pipes(SourceT source, const PipeTuple& pipes) {
      if constexpr (I == std::tuple_size_v<PipeTuple>) {
        return source;
      } else {
        return apply_pipes<I + 1>(std::get<I>(pipes)(std::move(source)), pipes);
      }
    }
  }

  // Composes N pipe factories into a single callable
  // that applies them left-to-right when given a source.
  // All type deduction is deferred until the composed pipe is called.
  //
  // Usage:
  //   auto pipe = compose_pipes(map(fn), filter(pred), dedupe());
  //   auto result = source | pipe;
  //
  // Can be combined with typed_pipe to satisfy the Pipe concept:
  //   auto pipe = typed_pipe<int>(compose_pipes(filter(pred), dedupe()));
  template <typename... PipeFns>
  struct ComposedPipe {
    using is_pipe_factory = void;
    std::tuple<PipeFns...> pipes;

    template <typename SourceT>
      requires concepts::Source<SourceT>
    RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
      return detail::apply_pipes<0>(std::move(source), pipes);
    }
  };

  template <typename... PipeFns>
  auto compose_pipes(PipeFns... pipes) {
    return ComposedPipe<std::decay_t<PipeFns>...>{std::make_tuple(std::move(pipes)...)};
  }

  // Wraps any callable into a pipe factory that can compose with |.
  // Analogous to source_fn<T>() for sources, but without type params
  // since pipe factories are intentionally untyped.
  template <typename Fn>
  struct PipeFactoryWrapper {
    using is_pipe_factory = void;
    Fn fn;

    template <typename SourceT>
      requires concepts::Source<SourceT>
    RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
      return fn(std::move(source));
    }
  };

  template <typename Fn>
  auto as_pipe(Fn fn) {
    return PipeFactoryWrapper<std::decay_t<Fn>>{std::move(fn)};
  }

  // Wraps any callable into a Source without type erasure.
  // Analogous to source_fn<T> but preserves the concrete callable type
  // for inlining and optimization.
  template <typename T, typename Fn>
  struct SourceWrapper {
    using value_type = T;
    Fn fn;

    template <typename PushFn>
      requires concepts::Visitor<PushFn, T>
    RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
      return fn(std::move(push));
    }
  };

  template <typename T, typename Fn>
  auto as_source(Fn fn) {
    return SourceWrapper<T, std::decay_t<Fn>>{std::move(fn)};
  }

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
