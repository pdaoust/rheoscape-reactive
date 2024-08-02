#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo {

  // Remember the last pushed value,
  // so it can be pulled any time you like.
  // This value will stick around and can be pulled
  // even after the upstream source is ended.

  template <typename T>
  source_fn<T> cache(source_fn<T> source) {
    return [source](push_fn<T> push, end_fn _) {
      auto lastSeenValue = std::make_shared<std::optional<T>>();
      pull_fn pull = source(
        [push, lastSeenValue](T value) {
          lastSeenValue = value;
          push(value);
        },
        [](){}
      );

      return [push, lastSeenValue]() {
        if (lastSeenValue.has_value()) {
          push(lastSeenValue.value());
        }
      }
    };
  }

}