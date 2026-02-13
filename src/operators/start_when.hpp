#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  template <typename T, typename FilterFn>
    requires concepts::Predicate<FilterFn, T>
  RHEO_CALLABLE source_fn<T> start_when(source_fn<T> source, FilterFn&& condition) {
    using FilterFnDecayed = std::decay_t<FilterFn>;

    struct SourceBinder {
      source_fn<T> source;
      FilterFnDecayed condition;

      RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {

        struct PushHandler {
          FilterFnDecayed condition;
          push_fn<T> push;
          mutable bool started = false;

          RHEO_CALLABLE void operator()(T value) const {
            if (!started && condition(value)) {
              started = true;
            }
            if (started) {
              push(value);
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
    struct StartWhenPipeFactory {
      FilterFn condition;

      template <typename T>
        requires concepts::Predicate<FilterFn, T>
      RHEO_CALLABLE auto operator()(source_fn<T> source) const {
        return start_when(std::move(source), FilterFn(condition));
      }
    };
  }

  template <typename FilterFn>
  auto start_when(FilterFn&& condition) {
    return detail::StartWhenPipeFactory<std::decay_t<FilterFn>>{
      std::forward<FilterFn>(condition)
    };
  }

}
