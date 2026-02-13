#pragma once

#include <functional>
#include <optional>
#include <core_types.hpp>

namespace rheo::operators {

  // Only push the first received instance of a value.
  //
  // PATTERN NOTE: This operator intentionally has no pipe factory (no zero-argument
  // dedupe() overload) because dedupe() with a source argument IS already a pipe
  // function - it takes source_fn<T> and returns source_fn<T>. You can use it
  // directly with the pipe operator: source | dedupe
  template <typename T>
  RHEO_CALLABLE source_fn<T> dedupe(source_fn<T> source) {

    struct SourceBinder {
      source_fn<T> source;

      RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {

        struct PushHandler {
          push_fn<T> push;
          std::shared_ptr<std::optional<T>> last_seen_value;

          PushHandler(push_fn<T> push)
            : push(push), last_seen_value(std::make_shared<std::optional<T>>(std::nullopt)) {}

          RHEO_CALLABLE void operator()(T value) const {
            if (!last_seen_value->has_value() || last_seen_value->value() != value) {
              last_seen_value->emplace(value);
              push(value);
            }
          }
        };

        return source(PushHandler{push});
      }
    };

    return SourceBinder{source};
  }

}
