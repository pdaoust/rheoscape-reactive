#pragma once

#include <functional>
#include <core_types.hpp>
#include <Endable.hpp>

namespace rheo::operators {

  // Re-emit values from the source function until a condition is met,
  // then end the source.

  template <typename T, typename FilterFn>
  source_fn<Endable<T>> takeWhile(source_fn<T> source, FilterFn&& condition) {
    return [source, condition = std::forward<FilterFn>(condition)](push_fn<Endable<T>> push) {
      return source([condition, push, running = true](T&& value) mutable {
        if (running) {
          if (!condition(value)) {
            running = false;
            push(Endable<T>());
          } else {
            push(Endable<T>(std::forward<T>(value)));
          }
        }
      });
    };
  }

  template <typename FilterFn>
  auto takeWhile(FilterFn&& condition)
  -> pipe_fn<Endable<transformer_1_in_in_type_t<std::decay_t<FilterFn>>>, transformer_1_in_in_type_t<std::decay_t<FilterFn>>> {
    using T = transformer_1_in_in_type_t<std::decay_t<FilterFn>>;
    return [condition = std::forward<FilterFn>(condition)](source_fn<T> source) {
      return takeWhile(source, condition);
    };
  }

  template <typename T, typename FilterFn>
  source_fn<Endable<T>> takeUntil(source_fn<T> source, FilterFn&& condition) {
    return takeWhile(source, [condition = std::forward<FilterFn>(condition)](T value) { return !condition(value); });
  }

  template <typename FilterFn>
  auto takeUntil(FilterFn&& condition) {
    using T = transformer_1_in_in_type_t<std::decay_t<FilterFn>>;
    return [condition = std::forward<FilterFn>(condition)](source_fn<T> source) {
      return takeUntil(source, condition);
    };
  }

}