#ifndef RHEOSCAPE_TOGGLE
#define RHEOSCAPE_TOGGLE

#include <functional>
#include <core_types.hpp>

// Toggle a value source on and off with a boolean toggle source.
// It's off until the toggle source pushes the first true value.
template <typename T>
source_fn<T> toggle(source_fn<T> valueSource, source_fn<bool> toggleSource) {
  return [valueSource, toggleSource](push_fn<T> push) {
    auto toggleState = std::make_shared<std::optional<bool>>();
    pull_fn pullToggleSource = toggleSource([toggleState](bool value) {
      toggleState->emplace(value);
    });

    pull_fn pullValueSource = valueSource([push, toggleState](T value) {
      if (toggleState->has_value() && toggleState->value()) {
        push(value);
      }
    });

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