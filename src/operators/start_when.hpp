#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  namespace detail {
    template <typename SourceT, typename FilterFnT>
    struct StartWhenSourceBinder {
      using value_type = source_value_t<SourceT>;

      SourceT source;
      FilterFnT condition;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {
        using T = value_type;

        struct PushHandler {
          FilterFnT condition;
          PushFn push;
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

        return source(PushHandler{condition, std::move(push)});
      }
    };
  }

  template <typename SourceT, typename FilterFn>
    requires concepts::Source<SourceT> && concepts::Predicate<FilterFn, source_value_t<SourceT>>
  RHEO_CALLABLE auto start_when(SourceT source, FilterFn&& condition) {
    return detail::StartWhenSourceBinder<SourceT, std::decay_t<FilterFn>>{
      std::move(source),
      std::forward<FilterFn>(condition)
    };
  }

  namespace detail {
    template <typename FilterFn>
    struct StartWhenPipeFactory {
      FilterFn condition;

      template <typename SourceT>
        requires concepts::Source<SourceT> && concepts::Predicate<FilterFn, source_value_t<SourceT>>
      RHEO_CALLABLE auto operator()(SourceT source) const {
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
