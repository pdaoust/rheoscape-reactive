#pragma once

#include <functional>
#include <optional>
#include <core_types.hpp>

namespace rheo::operators {

  // Named callable for dedupe's push handler
  // Uses shared_ptr for state so copies share the same last_seen_value
  template<typename T>
  struct dedupe_push_handler {
    push_fn<T> push;
    std::shared_ptr<std::optional<T>> last_seen_value;

    dedupe_push_handler(push_fn<T> push)
      : push(push), last_seen_value(std::make_shared<std::optional<T>>(std::nullopt)) {}

    RHEO_NOINLINE void operator()(T value) const {
      if (!last_seen_value->has_value() || last_seen_value->value() != value) {
        last_seen_value->emplace(value);
        push(value);
      }
    }
  };

  // Named callable for dedupe's source binder
  template<typename T>
  struct dedupe_source_binder {
    source_fn<T> source;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      return source(dedupe_push_handler<T>{push});
    }
  };

  // Only push the first received instance of a value.
  //
  // PATTERN NOTE: This operator intentionally has no pipe factory (no zero-argument
  // dedupe() overload) because dedupe() with a source argument IS already a pipe
  // function - it takes source_fn<T> and returns source_fn<T>. You can use it
  // directly with the pipe operator: source | dedupe
  template <typename T>
  RHEO_INLINE source_fn<T> dedupe(source_fn<T> source) {
    return dedupe_source_binder<T>{source};
  }

}