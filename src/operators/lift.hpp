#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/filter.hpp>
#include <operators/map.hpp>

namespace rheo {
  
  // Lift a pipe function to a higher-ordered type;
  // e.g., a pipe_fn<T, T> to a pipe<std::optional<T>, std::optional<T>>.
  template <typename TLifted, typename T>
  pipe_fn<TLifted, TLifted> lift(
    pipe_fn<T, T> pipe,
    map_fn<TLifted, T> liftFn,
    map_fn<T, TLifted> lowerFn,
    std::optional<filter_fn<TLifted>> canBeLowered = std::nullopt
  ) {
    return [pipe, liftFn, lowerFn, canBeLowered](source_fn<TLifted> source) {
      source_fn<T> lowered = canBeLowered.has_value()
        ? map(filter(source, canBeLowered.value()), lowerFn)
        : map(source, lowerFn);
      source_fn<T> processed = pipe(lowered);
      return map(liftFn);
    };
  }

  template <typename T>
  pipe_fn<std::optional<T>, std::optional<T>> liftToOptional(pipe_fn<T, T> pipe) {
    return lift(
      pipe,
      [](T value) { return (std::optional<T>)value; },
      [](std::optional<T> value) { return value.value(); },
      [](std::optional<T> value) { return value.has_value(); }
    );
  }

}