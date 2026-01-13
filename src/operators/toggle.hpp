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
    auto combined = combine(std::make_tuple<T, bool>, value_source, toggle_source);
    auto filtered = filter(combined, (rheo::filter_fn<std::tuple<T, bool>>)[](std::tuple<T, bool> value) { return std::get<1>(value); });
    return map(filtered, (map_fn<T, std::tuple<T, bool>>)[](std::tuple<T, bool> value) { return std::get<0>(value); });
  }

  // A pipe to toggle a value source by the given boolean toggle source.
  template <typename T>
  pipe_fn<T, T> toggle_on(source_fn<bool> toggle_source) {
    return [toggle_source](source_fn<T> value_source) {
      return toggle(value_source, toggle_source);
    };
  }

  // A pipe that uses a toggle source to toggle the given value source.
  template <typename T>
  pipe_fn<T, T> apply_toggle(source_fn<T> value_source) {
    return [value_source](source_fn<bool> toggle_source) {
      return toggle(value_source, toggle_source);
    };
  }

}