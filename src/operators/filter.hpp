#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  template <typename T, typename FilterFn>
    requires concepts::Predicate<FilterFn, T>
  RHEO_CALLABLE source_fn<T> filter(source_fn<T> source, FilterFn&& filterer) {
    using FilterFnDecayed = std::decay_t<FilterFn>;

    struct SourceBinder {
      source_fn<T> source;
      FilterFnDecayed filterer;

      RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {

        struct PushHandler {
          FilterFnDecayed filterer;
          push_fn<T> push;

          RHEO_CALLABLE void operator()(T value) const {
            if (filterer(value)) {
              push(value);
            }
          }
        };

        return source(PushHandler{filterer, std::move(push)});
      }
    };

    return SourceBinder{
      source,
      std::forward<FilterFn>(filterer)
    };
  }

  namespace detail {
    template <typename FilterFn>
    struct FilterPipeFactory {
      FilterFn filterer;

      template <typename T>
        requires concepts::Predicate<FilterFn, T>
      RHEO_CALLABLE auto operator()(source_fn<T> source) const {
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
