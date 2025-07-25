#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {
  
  template <typename T, typename FilterFn>
  source_fn<T> filter(source_fn<T> source, FilterFn&& filterer) {
    return [source, filterer = std::forward<FilterFn>(filterer)](push_fn<T> push) {
      return source([filterer, push](T value) { if (filterer(value)) push(value); });
    };
  }

  template <typename FilterFn>
  auto filter(FilterFn&& filterer)
  -> pipe_fn<transformer_1_in_in_type_t<std::decay_t<FilterFn>>, transformer_1_in_in_type_t<std::decay_t<FilterFn>>> {
    using T = transformer_1_in_in_type_t<std::decay_t<FilterFn>>;
    return [filterer = std::forward<FilterFn>(filterer)](source_fn<T> source) {
      return filter(source, filterer);
    };
  }

  // This is a filter_fn<std::optional<T>>.
  template <typename T>
  bool notEmpty(std::optional<T> value) {
    return value.has_value();
  }

}