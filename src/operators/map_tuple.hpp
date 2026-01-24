#pragma once

#include <functional>
#include <tuple>
#include <core_types.hpp>

namespace rheo::operators {

  // Map a tuple stream by destructuring the tuple and applying a mapper function.
  //
  // This operator takes a source of tuples and a mapper function,
  // applies the mapper to each tuple's elements via std::apply,
  // and pushes the result downstream.
  //
  // This is particularly useful in combination with `combine` or `sample`,
  // which produce tuple streams:
  //
  //   combine(source1, source2) | map_tuple([](T1 v1, T2 v2) { ... })
  //
  // The mapper function's arity must match the tuple size.

  // Named callable for map_tuple's push handler.
  template<typename TTuple, typename TOut, typename MapFn>
  struct map_tuple_push_handler {
    push_fn<TOut> push;
    // Mutable to support stateful mappers (mutable lambdas).
    mutable MapFn mapper;

    RHEO_NOINLINE void operator()(TTuple value) const {
      push(std::apply(mapper, std::move(value)));
    }
  };

  // Named callable for map_tuple's source binder.
  template<typename TTuple, typename TOut, typename MapFn>
  struct map_tuple_source_binder {
    source_fn<TTuple> source;
    MapFn mapper;

    RHEO_NOINLINE pull_fn operator()(push_fn<TOut> push) const {
      return source(map_tuple_push_handler<TTuple, TOut, MapFn>{std::move(push), mapper});
    }
  };

  // Source function factory: map_tuple(source, mapper)
  //
  // Takes a source of tuples and a mapper function,
  // returns a source that emits the result of applying the mapper to each tuple.
  template <typename TTuple, typename MapFn>
    requires concepts::TupleMapper<MapFn, TTuple>
  RHEO_INLINE auto map_tuple(source_fn<TTuple> source, MapFn&& mapper)
  -> source_fn<apply_result_t<std::decay_t<MapFn>, TTuple>> {
    using TOut = apply_result_t<std::decay_t<MapFn>, TTuple>;
    return map_tuple_source_binder<TTuple, TOut, std::decay_t<MapFn>>{
      std::move(source),
      std::forward<MapFn>(mapper)
    };
  }

  // Pipe factory overload: map_tuple(mapper)
  //
  // Returns a pipe function that can be used with the | operator.
  // Usage: combine(source1, source2) | map_tuple([](T1, T2) { ... })
  template <typename MapFn>
  auto map_tuple(MapFn&& mapper)
  -> pipe_fn<
      apply_result_t<std::decay_t<MapFn>, typename callable_traits<std::decay_t<MapFn>>::args_tuple>,
      typename callable_traits<std::decay_t<MapFn>>::args_tuple
    > {
    using TTuple = typename callable_traits<std::decay_t<MapFn>>::args_tuple;
    using TOut = apply_result_t<std::decay_t<MapFn>, TTuple>;
    return [mapper = std::forward<MapFn>(mapper)](source_fn<TTuple> source) -> source_fn<TOut> {
      return map_tuple(std::move(source), std::move(mapper));
    };
  }

}
