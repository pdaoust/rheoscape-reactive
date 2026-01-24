#pragma once

#include <functional>
#include <optional>
#include <core_types.hpp>

namespace rheo::operators {

  // Named callable for filter_map's push handler
  template<typename TIn, typename TOut, typename FilterMapFn>
  struct filter_map_push_handler {
    FilterMapFn filter_mapper;
    push_fn<TOut> push;

    RHEO_CALLABLE void operator()(TIn value) const {
      std::optional<TOut> maybe_mapped = filter_mapper(value);
      if (maybe_mapped.has_value()) {
        push(maybe_mapped.value());
      }
    }
  };

  // Named callable for filter_map's source binder
  template<typename TIn, typename TOut, typename FilterMapFn>
  struct filter_map_source_binder {
    source_fn<TIn> source;
    FilterMapFn filter_mapper;

    RHEO_CALLABLE pull_fn operator()(push_fn<TOut> push) const {
      return source(filter_map_push_handler<TIn, TOut, FilterMapFn>{filter_mapper, push});
    }
  };

  template <typename TIn, typename FilterMapFn>
    requires concepts::FilterMapper<FilterMapFn, TIn>
  auto filter_map(source_fn<TIn> source, FilterMapFn&& filter_mapper)
  -> source_fn<typename return_of<FilterMapFn>::value_type> {
    using TOut = typename return_of<FilterMapFn>::value_type;

    return filter_map_source_binder<TIn, TOut, std::decay_t<FilterMapFn>>{
      source,
      std::forward<FilterMapFn>(filter_mapper)
    };
  }

  // Pipe factory overload
  template <typename FilterMapFn>
  auto filter_map(FilterMapFn&& filter_mapper)
  -> pipe_fn<typename return_of<FilterMapFn>::value_type, arg_of<FilterMapFn>> {
    using TOut = typename return_of<FilterMapFn>::value_type;
    using TIn = arg_of<FilterMapFn>;
    return [filter_mapper = std::forward<FilterMapFn>(filter_mapper)](source_fn<TIn> source) -> source_fn<TOut> {
      return filter_map(std::move(source), std::move(filter_mapper));
    };
  }

}