#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  namespace detail {
    template <typename SourceT, typename FilterFnT>
    struct FilterSourceBinder {
      using value_type = source_value_t<SourceT>;

      SourceT source;
      FilterFnT filterer;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {
        using T = value_type;

        struct PushHandler {
          FilterFnT filterer;
          PushFn push;

          RHEO_CALLABLE void operator()(T value) const {
            if (filterer(value)) {
              push(value);
            }
          }
        };

        return source(PushHandler{filterer, std::move(push)});
      }
    };
  }

  template <typename SourceT, typename FilterFn>
    requires concepts::Source<SourceT> && concepts::Predicate<FilterFn, source_value_t<SourceT>>
  RHEO_CALLABLE auto filter(SourceT source, FilterFn&& filterer) {
    return detail::FilterSourceBinder<SourceT, std::decay_t<FilterFn>>{
      std::move(source),
      std::forward<FilterFn>(filterer)
    };
  }

  namespace detail {
    template <typename FilterFn>
    struct FilterPipeFactory {
      FilterFn filterer;

      template <typename SourceT>
        requires concepts::Source<SourceT> && concepts::Predicate<FilterFn, source_value_t<SourceT>>
      RHEO_CALLABLE auto operator()(SourceT source) const {
        return filter(std::move(source), FilterFn(filterer));
      }
    };
  }

  template <typename FilterFn>
  auto filter(FilterFn&& filterer) {
    return detail::FilterPipeFactory<std::decay_t<FilterFn>>{
      std::forward<FilterFn>(filterer)
    };
  }

  // This is a filter_fn<std::optional<T>>.
  template <typename T>
  bool not_empty(std::optional<T> value) {
    return value.has_value();
  }

}
