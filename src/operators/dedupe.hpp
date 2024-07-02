#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo {
  
  // Only push the first received instance of a value.
  // There is no factory for this function because it already is a pipe function!
  template <typename T>
  source_fn<T> dedupe(source_fn<T> source) {
    return [source](push_fn<T> push, end_fn end) {
      return source(
        [push, lastSeenValue = std::optional<T>()](T value) mutable {
          if (!lastSeenValue.has_value() || lastSeenValue.value() != value) {
            lastSeenValue = value;
            push(value);
          }
        },
        end
      );
    };
  }

}