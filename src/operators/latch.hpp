#pragma once

#include <functional>
#include <optional>
#include <core_types.hpp>

namespace rheo::operators {

  // Named callable for latch's push handler
  template<typename T>
  struct latch_push_handler {
    push_fn<T> push;
    mutable std::optional<T> lastSeenValue = std::nullopt;

    RHEO_NOINLINE void operator()(std::optional<T> value) const {
      if (value.has_value()) {
        lastSeenValue = value;
      }
      if (lastSeenValue.has_value()) {
        push(lastSeenValue.value());
      }
    }
  };

  // Named callable for latch's source binder
  template<typename T>
  struct latch_source_binder {
    source_fn<std::optional<T>> source;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      return source(latch_push_handler<T>{push});
    }
  };

  // Push the last non-empty value when pulled, even if the input source is empty.
  // Only starts pushing values once the first non-empty value has been received.
  // There is no factory for this function because it already is a pipe function!
  // If you only want to push non-empty values
  // rather than remembering the last non-empty value and pushing it when pulled,
  // use `filter<std::optional<T>>(source, notEmpty)`
  template <typename T>
  RHEO_INLINE source_fn<T> latch(source_fn<std::optional<T>> source) {
    return latch_source_binder<T>{source};
  }

}