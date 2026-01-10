#pragma once

#include <functional>
#include <core_types.hpp>
#include <Endable.hpp>

namespace rheo::operators {

  // Re-emit values from the source function until a condition is met,
  // then end the source.

  // Named callable for takeWhile's push handler
  template<typename T, typename FilterFn>
  struct takeWhile_push_handler {
    FilterFn condition;
    push_fn<Endable<T>> push;
    mutable bool running = true;

    RHEO_NOINLINE void operator()(T&& value) const {
      if (running) {
        if (!condition(value)) {
          running = false;
          push(Endable<T>());
        } else {
          push(Endable<T>(std::forward<T>(value)));
        }
      }
    }
  };

  // Named callable for takeWhile's source binder
  template<typename T, typename FilterFn>
  struct takeWhile_source_binder {
    source_fn<T> source;
    FilterFn condition;

    RHEO_NOINLINE pull_fn operator()(push_fn<Endable<T>> push) const {
      return source(takeWhile_push_handler<T, FilterFn>{condition, push});
    }
  };

  template <typename T, typename FilterFn>
    requires concepts::Predicate<FilterFn, T>
  RHEO_INLINE source_fn<Endable<T>> takeWhile(source_fn<T> source, FilterFn&& condition) {
    return takeWhile_source_binder<T, std::decay_t<FilterFn>>{
      source,
      std::forward<FilterFn>(condition)
    };
  }

  template <typename FilterFn>
  auto takeWhile(FilterFn&& condition)
  -> pipe_fn<Endable<arg_of<FilterFn>>, arg_of<FilterFn>> {
    using T = arg_of<FilterFn>;
    return [condition = std::forward<FilterFn>(condition)](source_fn<T> source) {
      return takeWhile(source, condition);
    };
  }

  // Named callable for takeUntil's negating predicate
  template<typename T, typename FilterFn>
  struct takeUntil_negating_predicate {
    FilterFn condition;

    RHEO_NOINLINE bool operator()(T value) const {
      return !condition(value);
    }
  };

  template <typename T, typename FilterFn>
    requires concepts::Predicate<FilterFn, T>
  RHEO_INLINE source_fn<Endable<T>> takeUntil(source_fn<T> source, FilterFn&& condition) {
    return takeWhile(source, takeUntil_negating_predicate<T, std::decay_t<FilterFn>>{std::forward<FilterFn>(condition)});
  }

  template <typename FilterFn>
  auto takeUntil(FilterFn&& condition) {
    using T = arg_of<FilterFn>;
    return [condition = std::forward<FilterFn>(condition)](source_fn<T> source) {
      return takeUntil(source, condition);
    };
  }

}