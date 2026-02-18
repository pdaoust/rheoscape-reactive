#pragma once

#include <functional>
#include <core_types.hpp>
#include <Endable.hpp>

namespace rheo::operators {

  namespace detail {
    template <typename SourceT, typename FilterFnT>
    struct TakeWhileSourceBinder {
      using T = source_value_t<SourceT>;
      using value_type = Endable<T>;

      SourceT source;
      FilterFnT condition;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {

        struct PushHandler {
          FilterFnT condition;
          PushFn push;
          mutable bool running = true;

          RHEO_CALLABLE void operator()(T&& value) const {
            if (running) {
              if (!invoke_maybe_apply(condition, value)) {
                running = false;
                push(Endable<T>());
              } else {
                push(Endable<T>(std::forward<T>(value)));
              }
            }
          }
        };

        return source(PushHandler{condition, std::move(push)});
      }
    };
  }

  // Re-emit values from the source function until a condition is met,
  // then end the source.

  template <typename SourceT, typename FilterFn>
    requires concepts::Source<SourceT> && concepts::Predicate<FilterFn, source_value_t<SourceT>>
  RHEO_CALLABLE auto take_while(SourceT source, FilterFn&& condition) {
    return detail::TakeWhileSourceBinder<SourceT, std::decay_t<FilterFn>>{
      std::move(source),
      std::forward<FilterFn>(condition)
    };
  }

  template <typename SourceT, typename FilterFn>
    requires concepts::Source<SourceT> && concepts::Predicate<FilterFn, source_value_t<SourceT>>
  RHEO_CALLABLE auto take_until(SourceT source, FilterFn&& condition) {
    using FilterFnDecayed = std::decay_t<FilterFn>;
    using T = source_value_t<SourceT>;

    struct NegatingPredicate {
      FilterFnDecayed condition;

      RHEO_CALLABLE bool operator()(T value) const {
        return !invoke_maybe_apply(condition, value);
      }
    };

    return take_while(std::move(source), NegatingPredicate{std::forward<FilterFn>(condition)});
  }

  namespace detail {
    template <typename FilterFn>
    struct TakeWhilePipeFactory {
      FilterFn condition;

      template <typename SourceT>
        requires concepts::Source<SourceT> && concepts::Predicate<FilterFn, source_value_t<SourceT>>
      RHEO_CALLABLE auto operator()(SourceT source) const {
        return take_while(std::move(source), FilterFn(condition));
      }
    };

    template <typename FilterFn>
    struct TakeUntilPipeFactory {
      FilterFn condition;

      template <typename SourceT>
        requires concepts::Source<SourceT> && concepts::Predicate<FilterFn, source_value_t<SourceT>>
      RHEO_CALLABLE auto operator()(SourceT source) const {
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

  template <typename FilterFn>
  auto take_until(FilterFn&& condition) {
    return detail::TakeUntilPipeFactory<std::decay_t<FilterFn>>{
      std::forward<FilterFn>(condition)
    };
  }

}
