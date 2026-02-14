#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/filter.hpp>
#include <operators/map.hpp>
#include <operators/combine.hpp>

namespace rheo::operators {

  // Toggle a value source on and off with a boolean toggle source.
  // It's off until the toggle source pushes the first true value.
  // This source ends when either of its sources ends.
  template <typename ValueSourceT, typename ToggleSourceT>
    requires concepts::Source<ValueSourceT> && concepts::Source<ToggleSourceT> &&
             std::is_same_v<source_value_t<ToggleSourceT>, bool>
  auto toggle(ValueSourceT value_source, ToggleSourceT toggle_source) {
    using T = source_value_t<ValueSourceT>;

    struct TogglePredicate {
      RHEO_CALLABLE bool operator()(std::tuple<T, bool> value) const {
        return std::get<1>(value);
      }
    };

    struct ValueExtractor {
      RHEO_CALLABLE T operator()(std::tuple<T, bool> value) const {
        return std::get<0>(value);
      }
    };

    auto combined = combine(std::move(value_source), std::move(toggle_source));
    auto filtered = filter(std::move(combined), TogglePredicate{});
    return map(std::move(filtered), ValueExtractor{});
  }

  namespace detail {
    template <typename ToggleSourceT>
    struct ToggleOnPipeFactory {
      ToggleSourceT toggle_source;

      template <typename ValueSourceT>
        requires concepts::Source<ValueSourceT>
      RHEO_CALLABLE auto operator()(ValueSourceT value_source) const {
        return toggle(std::move(value_source), ToggleSourceT(toggle_source));
      }
    };

    template <typename ValueSourceT>
    struct ApplyTogglePipeFactory {
      ValueSourceT value_source;

      template <typename ToggleSourceT>
        requires concepts::Source<ToggleSourceT> &&
                 std::is_same_v<source_value_t<ToggleSourceT>, bool>
      RHEO_CALLABLE auto operator()(ToggleSourceT toggle_source) const {
        return toggle(ValueSourceT(value_source), std::move(toggle_source));
      }
    };
  }

  // A pipe to toggle a value source by the given boolean toggle source.
  template <typename ToggleSourceT>
    requires concepts::Source<ToggleSourceT> &&
             std::is_same_v<source_value_t<ToggleSourceT>, bool>
  auto toggle_on(ToggleSourceT toggle_source) {
    return detail::ToggleOnPipeFactory<ToggleSourceT>{std::move(toggle_source)};
  }

  // A pipe that uses a toggle source to toggle the given value source.
  template <typename ValueSourceT>
    requires concepts::Source<ValueSourceT>
  auto apply_toggle(ValueSourceT value_source) {
    return detail::ApplyTogglePipeFactory<ValueSourceT>{std::move(value_source)};
  }

}
