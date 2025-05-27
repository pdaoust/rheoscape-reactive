#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/filter.hpp>
#include <operators/map.hpp>
#include <operators/zip.hpp>

namespace rheo::operators {

  // Toggle a value source on and off with a boolean toggle source.
  // It's off until the toggle source pushes the first true value.
  // This source ends when either of its sources ends.
  template <typename T>
  source_fn<T> toggle(source_fn<T> valueSource, source_fn<bool> toggleSource) {
    auto zipped = zipTuple(valueSource, toggleSource);
    auto filtered = filter(zipped, (rheo::filter_fn<std::tuple<T, bool>>)[](std::tuple<T, bool> value) { return std::get<1>(value); });
    return map(filtered, (map_fn<T, std::tuple<T, bool>>)[](std::tuple<T, bool> value) { return std::get<0>(value); });
  }

  // A pipe to toggle a value source by the given boolean toggle source.
  template <typename T>
  pipe_fn<T, T> toggleOn(source_fn<bool> toggleSource) {
    return [toggleSource](source_fn<T> valueSource) {
      return toggle(valueSource, toggleSource);
    };
  }

  // A pipe that uses a toggle source to toggle the given value source.
  template <typename T>
  pipe_fn<T, T> applyToggle(source_fn<T> valueSource) {
    return [valueSource](source_fn<bool> toggleSource) {
      return toggle(valueSource, toggleSource);
    };
  }

}