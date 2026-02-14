#pragma once
#include <functional>
#include <core_types.hpp>
#include <sources/constant.hpp>
#include <fmt/format.h>

namespace rheo::operators {

  namespace detail {
    template <typename SourceT, typename MapFnT>
    struct MapSourceBinder {
      using TIn = source_value_t<SourceT>;
      using value_type = std::invoke_result_t<MapFnT, TIn>;

      SourceT source;
      MapFnT mapper;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {

        struct PushHandler {
          PushFn push;
          MapFnT mapper;

          RHEO_CALLABLE void operator()(TIn value) const {
            push(mapper(value));
          }
        };

        return source(PushHandler{std::move(push), mapper});
      }
    };
  }

  template <typename SourceT, typename MapFn>
    requires concepts::Source<SourceT> && concepts::Transformer<MapFn, source_value_t<SourceT>>
  RHEO_CALLABLE auto map(SourceT source, MapFn&& mapper) {
    return detail::MapSourceBinder<SourceT, std::decay_t<MapFn>>{
      std::move(source),
      std::forward<MapFn>(mapper)
    };
  }

  namespace detail {
    template <typename MapFn>
    struct MapPipeFactory {
      MapFn mapper;

      template <typename SourceT>
        requires concepts::Source<SourceT> && concepts::Transformer<MapFn, source_value_t<SourceT>>
      RHEO_CALLABLE auto operator()(SourceT source) const {
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
