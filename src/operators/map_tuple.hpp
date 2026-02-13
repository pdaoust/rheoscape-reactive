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

  // Source function factory: map_tuple(source, mapper)
  //
  // Takes a source of tuples and a mapper function,
  // returns a source that emits the result of applying the mapper to each tuple.
  template <typename TTuple, typename MapFn>
    requires concepts::TupleMapper<MapFn, TTuple>
  RHEO_CALLABLE auto map_tuple(source_fn<TTuple> source, MapFn&& mapper)
  -> source_fn<apply_result_t<std::decay_t<MapFn>, TTuple>> {
    using TOut = apply_result_t<std::decay_t<MapFn>, TTuple>;
    using MapFnDecayed = std::decay_t<MapFn>;

    struct SourceBinder {
      source_fn<TTuple> source;
      MapFnDecayed mapper;

      RHEO_CALLABLE pull_fn operator()(push_fn<TOut> push) const {

        struct PushHandler {
          push_fn<TOut> push;
          // Mutable to support stateful mappers (mutable lambdas).
          mutable MapFnDecayed mapper;

          RHEO_CALLABLE void operator()(TTuple value) const {
            push(std::apply(mapper, std::move(value)));
          }
        };

        return source(PushHandler{std::move(push), mapper});
      }
    };

    return SourceBinder{
      std::move(source),
      std::forward<MapFn>(mapper)
    };
  }

  namespace detail {
    template <typename MapFn>
    struct MapTuplePipeFactory {
      MapFn mapper;

      template <typename TTuple>
        requires concepts::TupleMapper<MapFn, TTuple>
      RHEO_CALLABLE auto operator()(source_fn<TTuple> source) const {
        return map_tuple(std::move(source), MapFn(mapper));
      }
    };
  }

  template <typename MapFn>
  auto map_tuple(MapFn&& mapper) {
    return detail::MapTuplePipeFactory<std::decay_t<MapFn>>{
      std::forward<MapFn>(mapper)
    };
  }

}
