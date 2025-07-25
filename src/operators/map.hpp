#pragma once
#include <functional>
#include <core_types.hpp>
#include <sources/constant.hpp>
#include <fmt/format.h>

namespace rheo::operators {

  template <typename TIn, typename MapFn>
  auto map(source_fn<TIn> source, MapFn&& mapper)
  -> source_fn<transformer_1_in_out_type_t<std::decay_t<MapFn>>> {
    using OutputType = transformer_1_in_out_type_t<std::decay_t<MapFn>>;

    return [
      source,
      mapper = std::forward<MapFn>(mapper)
    ]
    (push_fn<OutputType> push) -> pull_fn {
      return source([
        push = std::move(push),
        mapper = std::move(mapper)
      ](TIn value) {
        push(mapper(value));
      });
    };
  }

  template <typename MapFn>
  auto map(MapFn&& mapper)
  -> pipe_fn<
    transformer_1_in_out_type_t<std::decay_t<MapFn>>,
    transformer_1_in_in_type_t<std::decay_t<MapFn>>
  > {
    using TOut = transformer_1_in_out_type_t<std::decay_t<MapFn>>;
    using TIn = transformer_1_in_in_type_t<std::decay_t<MapFn>>;

    return [mapper = std::forward<MapFn>(mapper)](source_fn<TIn> source) -> source_fn<TOut>{
      return map(std::move(source), std::move(mapper));
    };
  }

}