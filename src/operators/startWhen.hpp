#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  template <typename T, typename FilterFn>
  source_fn<T> startWhen(source_fn<T> source, FilterFn&& condition) {
    return [source, condition = std::forward<FilterFn>(condition)](push_fn<T> push) {
      bool started = false;
      return source([condition, push, started](T value) mutable {
        if (!started && condition(value)) {
          started = true;
        }
        if (started) {
          push(value);
        }
      });
    };
  }

  template <typename T, typename FilterFn>
  pipe_fn<T, T> startWhen(FilterFn&& condition) {
    return [condition = std::forward<FilterFn>(condition)](source_fn<T> source) {
      return startWhen(source, condition);
    };
  }

}