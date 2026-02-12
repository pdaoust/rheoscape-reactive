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
  template <typename T>
  source_fn<T> toggle(source_fn<T> value_source, source_fn<bool> toggle_source) {
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

    auto combined = combine(value_source, toggle_source);
    auto filtered = filter(combined, TogglePredicate{});
    return map(filtered, ValueExtractor{});
  }

  // A pipe to toggle a value source by the given boolean toggle source.
  template <typename T>
  pipe_fn<T, T> toggle_on(source_fn<bool> toggle_source) {
    struct PipeFactory {
      source_fn<bool> toggle_source;

      RHEO_CALLABLE source_fn<T> operator()(source_fn<T> value_source) const {
        return toggle(value_source, toggle_source);
      }
    };

    return PipeFactory{toggle_source};
  }

  // A pipe that uses a toggle source to toggle the given value source.
  template <typename T>
  pipe_fn<T, T> apply_toggle(source_fn<T> value_source) {
    struct PipeFactory {
      source_fn<T> value_source;

      RHEO_CALLABLE source_fn<T> operator()(source_fn<bool> toggle_source) const {
        return toggle(value_source, toggle_source);
      }
    };

    return PipeFactory{value_source};
  }

}
