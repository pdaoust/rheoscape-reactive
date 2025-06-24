#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {
  
  template <typename TOut, typename TIn>
  source_fn<TOut> filterMap(source_fn<TIn> source, filter_map_fn<TOut, TIn> filterMapper) {
    return [source, filterMapper](push_fn<TOut> push, end_fn end) {
      return source(
        [filterMapper, push](TIn value) {
          std::optional<TOut> maybeMapped = filterMapper(value);
          if (maybeMapped.has_value()) {
            push(maybeMapped.value());
          }
        },
        end
      );
    };
  }

  template <typename TOut, typename TIn>
  pipe_fn<TOut, TIn> filterMap(filter_map_fn<TOut, TIn> filterMapper) {
    return [filterMapper](source_fn<TIn> source) {
      return filterMap(source, filterMapper);
    };
  }

}