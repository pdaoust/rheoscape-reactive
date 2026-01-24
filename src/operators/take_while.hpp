#pragma once

#include <functional>
#include <core_types.hpp>
#include <Endable.hpp>

namespace rheo::operators {

  // Re-emit values from the source function until a condition is met,
  // then end the source.

  // Named callable for take_while's push handler
  template<typename T, typename FilterFn>
  struct take_while_push_handler {
    FilterFn condition;
    push_fn<Endable<T>> push;
    mutable bool running = true;

    RHEO_CALLABLE void operator()(T&& value) const {
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

  // Named callable for take_while's source binder
  template<typename T, typename FilterFn>
  struct take_while_source_binder {
    source_fn<T> source;
    FilterFn condition;

    RHEO_CALLABLE pull_fn operator()(push_fn<Endable<T>> push) const {
      return source(take_while_push_handler<T, FilterFn>{condition, push});
    }
  };

  template <typename T, typename FilterFn>
    requires concepts::Predicate<FilterFn, T>
  RHEO_CALLABLE source_fn<Endable<T>> take_while(source_fn<T> source, FilterFn&& condition) {
    return take_while_source_binder<T, std::decay_t<FilterFn>>{
      source,
      std::forward<FilterFn>(condition)
    };
  }

  template <typename FilterFn>
  auto take_while(FilterFn&& condition)
  -> pipe_fn<Endable<arg_of<FilterFn>>, arg_of<FilterFn>> {
    using T = arg_of<FilterFn>;
    return [condition = std::forward<FilterFn>(condition)](source_fn<T> source) {
      return take_while(source, condition);
    };
  }

  // Named callable for take_until's negating predicate
  template<typename T, typename FilterFn>
  struct take_until_negating_predicate {
    FilterFn condition;

    RHEO_CALLABLE bool operator()(T value) const {
      return !condition(value);
    }
  };

  template <typename T, typename FilterFn>
    requires concepts::Predicate<FilterFn, T>
  RHEO_CALLABLE source_fn<Endable<T>> take_until(source_fn<T> source, FilterFn&& condition) {
    return take_while(source, take_until_negating_predicate<T, std::decay_t<FilterFn>>{std::forward<FilterFn>(condition)});
  }

  template <typename FilterFn>
  auto take_until(FilterFn&& condition) {
    using T = arg_of<FilterFn>;
    return [condition = std::forward<FilterFn>(condition)](source_fn<T> source) {
      return take_until(source, condition);
    };
  }

}