#ifndef RHEOSCAPE_TOGGLE
#define RHEOSCAPE_TOGGLE

#include <functional>
#include <core_types.hpp>
#include <util.hpp>

// Toggle a value source on and off with a boolean toggle source.
// It's off until the toggle source pushes the first true value.
// This source ends when either of its sources ends.
template <typename T>
source_fn<T> toggle(source_fn<T> valueSource, source_fn<bool> toggleSource) {
  return [valueSource, toggleSource](push_fn<T> push, end_fn end) {
    auto toggleState = std::make_shared<std::optional<bool>>();
    auto endAny = std::make_shared<EndAny>();

    pull_fn pullToggleSource = toggleSource(
      [toggleState, endAny](bool value) {
        if (endAny->ended) {
          return;
        }
        toggleState->emplace(value);
      },
      endAny->upstream_end_fn
    );

    pull_fn pullValueSource = valueSource(
      [push, toggleState, endAny](T value) {
        if (!endAny->ended && toggleState->has_value() && toggleState->value()) {
          push(value);
        }
      },
      endAny->upstream_end_fn
    );

    return [pullToggleSource, pullValueSource]() {
      // Pull this one first to give us a better chance of getting a desired toggle state.
      pullToggleSource();
      pullValueSource();
    };
  };
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

#endif