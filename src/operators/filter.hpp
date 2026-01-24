#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  // Named callable for filter's push handler - provides better stack traces in debug mode
  template<typename T, typename FilterFn>
  struct filter_push_handler {
    FilterFn filterer;
    push_fn<T> push;

    RHEO_CALLABLE void operator()(T value) const {
      if (filterer(value)) {
        push(value);
      }
    }
  };

  // Named callable for filter's source binder
  template<typename T, typename FilterFn>
  struct filter_source_binder {
    source_fn<T> source;
    FilterFn filterer;

    RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {
      return source(filter_push_handler<T, FilterFn>{filterer, std::move(push)});
    }
  };

  template <typename T, typename FilterFn>
    requires concepts::Predicate<FilterFn, T>
  RHEO_CALLABLE source_fn<T> filter(source_fn<T> source, FilterFn&& filterer) {
    return filter_source_binder<T, std::decay_t<FilterFn>>{
      source,
      std::forward<FilterFn>(filterer)
    };
  }

  // Pipe factory overload
  template <typename FilterFn>
  auto filter(FilterFn&& filterer)
  -> pipe_fn<arg_of<FilterFn>, arg_of<FilterFn>> {
    using T = arg_of<FilterFn>;
    return [filterer = std::forward<FilterFn>(filterer)](source_fn<T> source) -> source_fn<T> {
      return filter(std::move(source), std::move(filterer));
    };
  }

  // This is a filter_fn<std::optional<T>>.
  template <typename T>
  bool not_empty(std::optional<T> value) {
    return value.has_value();
  }

}