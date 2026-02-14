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

  namespace detail {
    template <typename SourceT, typename MapFnT>
    struct MapTupleSourceBinder {
      using TTuple = source_value_t<SourceT>;
      using value_type = apply_result_t<MapFnT, TTuple>;

      SourceT source;
      MapFnT mapper;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {
        using TOut = value_type;

        struct PushHandler {
          PushFn push;
          // Mutable to support stateful mappers (mutable lambdas).
          mutable MapFnT mapper;

          RHEO_CALLABLE void operator()(TTuple value) const {
            push(std::apply(mapper, std::move(value)));
          }
        };

        return source(PushHandler{std::move(push), mapper});
      }
    };
  }

  // Source function factory: map_tuple(source, mapper)
  //
  // Takes a source of tuples and a mapper function,
  // returns a source that emits the result of applying the mapper to each tuple.
  template <typename SourceT, typename MapFn>
    requires concepts::Source<SourceT> && concepts::TupleMapper<MapFn, source_value_t<SourceT>>
  RHEO_CALLABLE auto map_tuple(SourceT source, MapFn&& mapper) {
    return detail::MapTupleSourceBinder<SourceT, std::decay_t<MapFn>>{
      std::move(source),
      std::forward<MapFn>(mapper)
    };
  }

  namespace detail {
    template <typename MapFn>
    struct MapTuplePipeFactory {
      MapFn mapper;

      template <typename SourceT>
        requires concepts::Source<SourceT> && concepts::TupleMapper<MapFn, source_value_t<SourceT>>
      RHEO_CALLABLE auto operator()(SourceT source) const {
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
