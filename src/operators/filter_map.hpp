#pragma once

#include <functional>
#include <optional>
#include <core_types.hpp>

namespace rheo::operators {

  template <typename TIn, typename FilterMapFn>
    requires concepts::FilterMapper<FilterMapFn, TIn>
  auto filter_map(source_fn<TIn> source, FilterMapFn&& filter_mapper)
  -> source_fn<typename return_of<FilterMapFn>::value_type> {
    using TOut = typename return_of<FilterMapFn>::value_type;
    using FilterMapFnDecayed = std::decay_t<FilterMapFn>;

    struct SourceBinder {
      source_fn<TIn> source;
      FilterMapFnDecayed filter_mapper;

      RHEO_CALLABLE pull_fn operator()(push_fn<TOut> push) const {

        struct PushHandler {
          FilterMapFnDecayed filter_mapper;
          push_fn<TOut> push;

          RHEO_CALLABLE void operator()(TIn value) const {
            std::optional<TOut> maybe_mapped = filter_mapper(value);
            if (maybe_mapped.has_value()) {
              push(maybe_mapped.value());
            }
          }
        };

        return source(PushHandler{filter_mapper, push});
      }
    };

    return SourceBinder{
      source,
      std::forward<FilterMapFn>(filter_mapper)
    };
  }

  namespace detail {
    template <typename FilterMapFn>
    struct FilterMapPipeFactory {
      FilterMapFn filter_mapper;

      template <typename TIn>
        requires concepts::FilterMapper<FilterMapFn, TIn>
      RHEO_CALLABLE auto operator()(source_fn<TIn> source) const {
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
