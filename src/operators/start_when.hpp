#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  // Named callable for start_when's push handler
  template<typename T, typename FilterFn>
  struct start_when_push_handler {
    FilterFn condition;
    push_fn<T> push;
    mutable bool started = false;

    RHEO_NOINLINE void operator()(T value) const {
      if (!started && condition(value)) {
        started = true;
      }
      if (started) {
        push(value);
      }
    }
  };

  // Named callable for start_when's source binder
  template<typename T, typename FilterFn>
  struct start_when_source_binder {
    source_fn<T> source;
    FilterFn condition;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      return source(start_when_push_handler<T, FilterFn>{condition, push});
    }
  };

  template <typename T, typename FilterFn>
    requires concepts::Predicate<FilterFn, T>
  RHEO_INLINE source_fn<T> start_when(source_fn<T> source, FilterFn&& condition) {
    return start_when_source_binder<T, std::decay_t<FilterFn>>{
      source,
      std::forward<FilterFn>(condition)
    };
  }

  template <typename T, typename FilterFn>
    requires concepts::Predicate<FilterFn, T>
  RHEO_INLINE pipe_fn<T, T> start_when(FilterFn&& condition) {
    return [condition = std::forward<FilterFn>(condition)](source_fn<T> source) {
      return start_when(source, condition);
    };
  }

}