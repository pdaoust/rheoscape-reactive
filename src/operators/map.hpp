#pragma once
#include <functional>
#include <types/core_types.hpp>

namespace rheoscape::operators {

  namespace detail {
    template <typename SourceT, typename MapFnT>
    struct MapSourceBinder {
      using TIn = source_value_t<SourceT>;
      using value_type = invoke_maybe_apply_result_t<MapFnT, TIn>;

      SourceT source;
      MapFnT mapper;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {

        struct PushHandler {
          PushFn push;
          MapFnT mapper;

          RHEOSCAPE_CALLABLE void operator()(TIn value) const {
            push(invoke_maybe_apply(mapper, std::move(value)));
          }
        };

        return source(PushHandler{std::move(push), mapper});
      }
    };
  }

  template <typename SourceT, typename MapFn>
    requires concepts::Source<SourceT> && concepts::Transformer<MapFn, source_value_t<SourceT>>
  RHEOSCAPE_CALLABLE auto map(SourceT source, MapFn&& mapper) {
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
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
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
