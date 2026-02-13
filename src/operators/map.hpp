#pragma once
#include <functional>
#include <core_types.hpp>
#include <sources/constant.hpp>
#include <fmt/format.h>

namespace rheo::operators {

  template <typename TIn, typename MapFn>
    requires concepts::Transformer<MapFn, TIn>
  RHEO_CALLABLE auto map(source_fn<TIn> source, MapFn&& mapper)
  -> source_fn<return_of<MapFn>> {
    using TOut = return_of<MapFn>;
    using MapFnDecayed = std::decay_t<MapFn>;

    struct SourceBinder {
      source_fn<TIn> source;
      MapFnDecayed mapper;

      RHEO_CALLABLE pull_fn operator()(push_fn<TOut> push) const {

        struct PushHandler {
          push_fn<TOut> push;
          MapFnDecayed mapper;

          RHEO_CALLABLE void operator()(TIn value) const {
            push(mapper(value));
          }
        };

        return source(PushHandler{std::move(push), mapper});
      }
    };

    return SourceBinder{
      source,
      std::forward<MapFn>(mapper)
    };
  }

  namespace detail {
    template <typename MapFn>
    struct MapPipeFactory {
      MapFn mapper;

      template <typename TIn>
        requires concepts::Transformer<MapFn, TIn>
      RHEO_CALLABLE auto operator()(source_fn<TIn> source) const {
        return map(std::move(source), MapFn(mapper));
      }
    };
  }

  template <typename MapFn>
  auto map(MapFn&& mapper) {
    return detail::MapPipeFactory<std::decay_t<MapFn>>{
      std::forward<MapFn>(mapper)
    };
  }

}
