#pragma once

#include <functional>
#include <optional>
#include <core_types.hpp>

namespace rheo::operators {

  // Named callable for filterMap's push handler
  template<typename TIn, typename TOut, typename FilterMapFn>
  struct filterMap_push_handler {
    FilterMapFn filterMapper;
    push_fn<TOut> push;

    RHEO_NOINLINE void operator()(TIn value) const {
      std::optional<TOut> maybeMapped = filterMapper(value);
      if (maybeMapped.has_value()) {
        push(maybeMapped.value());
      }
    }
  };

  // Named callable for filterMap's source binder
  template<typename TIn, typename TOut, typename FilterMapFn>
  struct filterMap_source_binder {
    source_fn<TIn> source;
    FilterMapFn filterMapper;

    RHEO_NOINLINE pull_fn operator()(push_fn<TOut> push) const {
      return source(filterMap_push_handler<TIn, TOut, FilterMapFn>{filterMapper, push});
    }
  };

  template <typename TIn, typename FilterMapFn>
    requires concepts::FilterMapper<FilterMapFn, TIn>
  auto filterMap(source_fn<TIn> source, FilterMapFn&& filterMapper)
  -> source_fn<typename return_of<FilterMapFn>::value_type> {
    using TOut = typename return_of<FilterMapFn>::value_type;

    return filterMap_source_binder<TIn, TOut, std::decay_t<FilterMapFn>>{
      source,
      std::forward<FilterMapFn>(filterMapper)
    };
  }

  // Pipe factory overload
  template <typename FilterMapFn>
  auto filterMap(FilterMapFn&& filterMapper)
  -> pipe_fn<typename return_of<FilterMapFn>::value_type, arg_of<FilterMapFn>> {
    using TOut = typename return_of<FilterMapFn>::value_type;
    using TIn = arg_of<FilterMapFn>;
    return [filterMapper = std::forward<FilterMapFn>(filterMapper)](source_fn<TIn> source) -> source_fn<TOut> {
      return filterMap(std::move(source), std::move(filterMapper));
    };
  }

}