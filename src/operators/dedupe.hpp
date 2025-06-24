#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/Wrapper.hpp>

namespace rheo::operators {
  
  // Only push the first received instance of a value.
  // There is no factory for this function because it already is a pipe function!
  template <typename T>
  source_fn<T> dedupe(source_fn<T> source) {
    return [source](push_fn<T> push) {
      auto lastSeenValue = rheo::make_wrapper_shared<std::optional<T>>(std::nullopt);
      return source([push, lastSeenValue](T value) {
        if (!lastSeenValue->value.has_value() || lastSeenValue->value.value() != value) {
          lastSeenValue->value.emplace(value);
          push(value);
        }
      });
    };
  }

}