#pragma once

#include <functional>
#include <optional>
#include <types/core_types.hpp>

namespace rheoscape::operators {

  // Push the last non-empty value when pulled, even if the input source is empty.
  // Only starts pushing values once the first non-empty value has been received.
  //
  // PATTERN NOTE: This operator intentionally has no pipe factory (no zero-argument
  // latch() overload) because latch() with a source argument IS already a pipe
  // function - it takes a source of optional<T> and returns a source of T. You can
  // use it directly with the pipe operator: optional_source | latch()
  //
  // If you only want to push non-empty values
  // rather than remembering the last non-empty value and pushing it when pulled,
  // use `filter<std::optional<T>>(source, not_empty)`

  namespace detail {
    template <typename SourceT>
    struct LatchSourceBinder {
      using OptT = source_value_t<SourceT>;
      using value_type = typename OptT::value_type;

      SourceT source;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        using T = value_type;

        struct PushHandler {
          PushFn push;
          mutable std::optional<T> last_seen_value = std::nullopt;

          RHEOSCAPE_CALLABLE void operator()(OptT value) const {
            if (value.has_value()) {
              last_seen_value = value;
            }
            if (last_seen_value.has_value()) {
              push(last_seen_value.value());
            }
          }
        };

        return source(PushHandler{std::move(push)});
      }
    };
  }

  template <typename SourceT>
    requires concepts::Source<SourceT> && is_optional_v<source_value_t<SourceT>>
  RHEOSCAPE_CALLABLE auto latch(SourceT source) {
    return detail::LatchSourceBinder<SourceT>{std::move(source)};
  }

  namespace detail {
    struct LatchPipeFactory {
      template <typename SourceT>
        requires concepts::Source<SourceT> && is_optional_v<source_value_t<SourceT>>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return latch(std::move(source));
      }
    };
  }

  inline auto latch() {
    return detail::LatchPipeFactory{};
  }

}
