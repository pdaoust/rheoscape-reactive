#pragma once

#include <functional>
#include <optional>
#include <core_types.hpp>

namespace rheo::operators {

  namespace detail {
    template <typename SourceT, typename FilterMapFnT>
    struct FilterMapSourceBinder {
      using TIn = source_value_t<SourceT>;
      using value_type = typename invoke_maybe_apply_result_t<FilterMapFnT, TIn>::value_type;

      SourceT source;
      FilterMapFnT filter_mapper;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {
        using TOut = value_type;

        struct PushHandler {
          FilterMapFnT filter_mapper;
          PushFn push;

          RHEO_CALLABLE void operator()(TIn value) const {
            std::optional<TOut> maybe_mapped = invoke_maybe_apply(filter_mapper, std::move(value));
            if (maybe_mapped.has_value()) {
              push(maybe_mapped.value());
            }
          }
        };

        return source(PushHandler{filter_mapper, std::move(push)});
      }
    };
  }

  template <typename SourceT, typename FilterMapFn>
    requires concepts::Source<SourceT> && concepts::FilterMapper<FilterMapFn, source_value_t<SourceT>>
  RHEO_CALLABLE auto filter_map(SourceT source, FilterMapFn&& filter_mapper) {
    return detail::FilterMapSourceBinder<SourceT, std::decay_t<FilterMapFn>>{
      std::move(source),
      std::forward<FilterMapFn>(filter_mapper)
    };
  }

  namespace detail {
    template <typename FilterMapFn>
    struct FilterMapPipeFactory {
      FilterMapFn filter_mapper;

      template <typename SourceT>
        requires concepts::Source<SourceT> && concepts::FilterMapper<FilterMapFn, source_value_t<SourceT>>
      RHEO_CALLABLE auto operator()(SourceT source) const {
        return filter_map(std::move(source), FilterMapFn(filter_mapper));
      }
    };
  }

  template <typename FilterMapFn>
  auto filter_map(FilterMapFn&& filter_mapper) {
    return detail::FilterMapPipeFactory<std::decay_t<FilterMapFn>>{
      std::forward<FilterMapFn>(filter_mapper)
    };
  }

}
