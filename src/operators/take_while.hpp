#pragma once

#include <functional>
#include <core_types.hpp>
#include <Endable.hpp>

namespace rheo::operators {

  // Re-emit values from the source function until a condition is met,
  // then end the source.

  template <typename T, typename FilterFn>
    requires concepts::Predicate<FilterFn, T>
  RHEO_CALLABLE source_fn<Endable<T>> take_while(source_fn<T> source, FilterFn&& condition) {
    using FilterFnDecayed = std::decay_t<FilterFn>;

    struct SourceBinder {
      source_fn<T> source;
      FilterFnDecayed condition;

      RHEO_CALLABLE pull_fn operator()(push_fn<Endable<T>> push) const {

        struct PushHandler {
          FilterFnDecayed condition;
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

        return source(PushHandler{condition, push});
      }
    };

    return SourceBinder{
      source,
      std::forward<FilterFn>(condition)
    };
  }

  namespace detail {
    template <typename FilterFn>
    struct TakeWhilePipeFactory {
      FilterFn condition;

      template <typename T>
        requires concepts::Predicate<FilterFn, T>
      RHEO_CALLABLE auto operator()(source_fn<T> source) const {
        return take_while(std::move(source), FilterFn(condition));
      }
    };

    template <typename FilterFn>
    struct TakeUntilPipeFactory {
      FilterFn condition;

      template <typename T>
        requires concepts::Predicate<FilterFn, T>
      RHEO_CALLABLE auto operator()(source_fn<T> source) const {
        return take_until(std::move(source), FilterFn(condition));
      }
    };
  }

  template <typename FilterFn>
  auto take_while(FilterFn&& condition) {
    return detail::TakeWhilePipeFactory<std::decay_t<FilterFn>>{
      std::forward<FilterFn>(condition)
    };
  }

  template <typename T, typename FilterFn>
    requires concepts::Predicate<FilterFn, T>
  RHEO_CALLABLE source_fn<Endable<T>> take_until(source_fn<T> source, FilterFn&& condition) {
    using FilterFnDecayed = std::decay_t<FilterFn>;

    struct NegatingPredicate {
      FilterFnDecayed condition;

      RHEO_CALLABLE bool operator()(T value) const {
        return !condition(value);
      }
    };

    return take_while(source, NegatingPredicate{std::forward<FilterFn>(condition)});
  }

  template <typename FilterFn>
  auto take_until(FilterFn&& condition) {
    return detail::TakeUntilPipeFactory<std::decay_t<FilterFn>>{
      std::forward<FilterFn>(condition)
    };
  }

}
