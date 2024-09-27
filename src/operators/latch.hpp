#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo {

  // Push the last non-null value when pulled, even if the input source is null.
  // Only starts pushing values once the first non-null value has been received.
  // There is no factory for this function because it already is a pipe function!
  // If you only want to push non-null values
  // rather than remembering the last non-null value and pushing it when pulled,
  // use `filter<std::optional<T>>(source, notEmpty)`
  template <typename T>
  source_fn<T> latch(source_fn<std::optional<T>> source) {
    return [source](push_fn<T> push, end_fn end) {
      std::optional<T> lastSeenValue;
      return source(
        [push, lastSeenValue](std::optional<T> value) mutable {
          if (value.has_value()) {
            lastSeenValue = value;
          }
          if (lastSeenValue.has_value()) {
            push(lastSeenValue.value());
          }
        },
        end
      );
    };
  }

}