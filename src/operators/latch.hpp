#pragma once

#include <functional>
#include <optional>
#include <core_types.hpp>

namespace rheo::operators {

  // Push the last non-empty value when pulled, even if the input source is empty.
  // Only starts pushing values once the first non-empty value has been received.
  //
  // PATTERN NOTE: This operator intentionally has no pipe factory (no zero-argument
  // latch() overload) because latch() with a source argument IS already a pipe
  // function - it takes source_fn<optional<T>> and returns source_fn<T>. You can
  // use it directly with the pipe operator: optional_source | latch
  //
  // If you only want to push non-empty values
  // rather than remembering the last non-empty value and pushing it when pulled,
  // use `filter<std::optional<T>>(source, not_empty)`
  template <typename T>
  RHEO_CALLABLE source_fn<T> latch(source_fn<std::optional<T>> source) {

    struct SourceBinder {
      source_fn<std::optional<T>> source;

      RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {

        struct PushHandler {
          push_fn<T> push;
          mutable std::optional<T> last_seen_value = std::nullopt;

          RHEO_CALLABLE void operator()(std::optional<T> value) const {
            if (value.has_value()) {
              last_seen_value = value;
            }
            if (last_seen_value.has_value()) {
              push(last_seen_value.value());
            }
          }
        };

        return source(PushHandler{push});
      }
    };

    return SourceBinder{source};
  }

}
