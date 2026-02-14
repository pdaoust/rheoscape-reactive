#pragma once

#include <functional>
#include <optional>
#include <memory>
#include <core_types.hpp>

namespace rheo::operators {

  // Only push the first received instance of a value.
  // This essentially turns a continuous value stream into a stream of change events.
  //
  // Usage: dedupe(source) or source | dedupe()

  namespace detail {
    template <typename SourceT>
    struct DedupeSourceBinder {
      using value_type = source_value_t<SourceT>;

      SourceT source;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {
        using T = value_type;

        struct PushHandler {
          PushFn push;
          std::shared_ptr<std::optional<T>> last_seen_value;

          PushHandler(PushFn push)
            : push(std::move(push)),
              last_seen_value(std::make_shared<std::optional<T>>(std::nullopt)) {}

          RHEO_CALLABLE void operator()(T value) const {
            if (!last_seen_value->has_value() || last_seen_value->value() != value) {
              last_seen_value->emplace(value);
              push(value);
            }
          }
        };

        return source(PushHandler{std::move(push)});
      }
    };
  }

  template <typename SourceT>
    requires concepts::Source<SourceT>
  RHEO_CALLABLE auto dedupe(SourceT source) {
    return detail::DedupeSourceBinder<SourceT>{std::move(source)};
  }

  namespace detail {
    struct DedupePipeFactory {
      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEO_CALLABLE auto operator()(SourceT source) const {
        return dedupe(std::move(source));
      }
    };
  }

  inline auto dedupe() {
    return detail::DedupePipeFactory{};
  }

}
