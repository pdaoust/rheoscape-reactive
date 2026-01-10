#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  // Named callable for startWhen's push handler
  template<typename T, typename FilterFn>
  struct startWhen_push_handler {
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

  // Named callable for startWhen's source binder
  template<typename T, typename FilterFn>
  struct startWhen_source_binder {
    source_fn<T> source;
    FilterFn condition;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      return source(startWhen_push_handler<T, FilterFn>{condition, push});
    }
  };

  template <typename T, typename FilterFn>
    requires concepts::Predicate<FilterFn, T>
  RHEO_INLINE source_fn<T> startWhen(source_fn<T> source, FilterFn&& condition) {
    return startWhen_source_binder<T, std::decay_t<FilterFn>>{
      source,
      std::forward<FilterFn>(condition)
    };
  }

  template <typename T, typename FilterFn>
    requires concepts::Predicate<FilterFn, T>
  RHEO_INLINE pipe_fn<T, T> startWhen(FilterFn&& condition) {
    return [condition = std::forward<FilterFn>(condition)](source_fn<T> source) {
      return startWhen(source, condition);
    };
  }

}