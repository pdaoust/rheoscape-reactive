#pragma once
#include <functional>
#include <core_types.hpp>
#include <sources/constant.hpp>
#include <fmt/format.h>

namespace rheo::operators {

  // Named callable for map's push handler - provides better stack traces in debug mode
  template<typename TIn, typename TOut, typename MapFn>
  struct map_push_handler {
    push_fn<TOut> push;
    MapFn mapper;

    RHEO_NOINLINE void operator()(TIn value) const {
      push(mapper(value));
    }
  };

  // Named callable for map's source binder
  template<typename TIn, typename TOut, typename MapFn>
  struct map_source_binder {
    source_fn<TIn> source;
    MapFn mapper;

    RHEO_NOINLINE pull_fn operator()(push_fn<TOut> push) const {
      return source(map_push_handler<TIn, TOut, MapFn>{std::move(push), mapper});
    }
  };

  template <typename TIn, typename MapFn>
    requires concepts::Transformer<MapFn, TIn>
  RHEO_INLINE auto map(source_fn<TIn> source, MapFn&& mapper)
  -> source_fn<return_of<MapFn>> {
    using TOut = return_of<MapFn>;

    return map_source_binder<TIn, TOut, std::decay_t<MapFn>>{
      source,
      std::forward<MapFn>(mapper)
    };
  }

  // Pipe factory overload
  template <typename MapFn>
  auto map(MapFn&& mapper)
  -> pipe_fn<return_of<MapFn>, arg_of<MapFn>> {
    using TOut = return_of<MapFn>;
    using TIn = arg_of<MapFn>;
    return [mapper = std::forward<MapFn>(mapper)](source_fn<TIn> source) -> source_fn<TOut> {
      return map(std::move(source), std::move(mapper));
    };
  }

}