#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {
  
  template <typename TIn, typename FilterMapFn>
  auto filterMap(source_fn<TIn> source, FilterMapFn&& filterMapper)
  -> source_fn<filter_mapper_wrapped_out_type_t<std::decay_t<FilterMapFn>>> {
    using TOut = filter_mapper_wrapped_out_type_t<std::decay_t<FilterMapFn>>;

    return [source, filterMapper = std::forward<FilterMapFn>(filterMapper)](push_fn<TOut> push) {
      return source(
        [filterMapper, push](TIn value) {
          std::optional<TOut> maybeMapped = filterMapper(value);
          if (maybeMapped.has_value()) {
            push(maybeMapped.value());
          }
        }
      );
    };
  }

  template <typename FilterMapFn>
  auto filterMap(FilterMapFn&& filterMapper)
  -> pipe_fn<transformer_1_in_out_type_t<std::decay_t<FilterMapFn>>, transformer_1_in_in_type_t<std::decay_t<FilterMapFn>>> {
    using TIn = transformer_1_in_in_type_t<std::decay_t<FilterMapFn>>;

    return [filterMapper = std::forward<FilterMapFn>(filterMapper)](source_fn<TIn> source) {
      return filterMap(source, filterMapper);
    };
  }

}