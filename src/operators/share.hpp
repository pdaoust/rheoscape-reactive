#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <types/core_types.hpp>

namespace rheoscape::operators {

  // Sharing a stream is like tapping it,
  // except that when any sink pulls, the value gets pushed to all sinks.
  // This function is useful for sharing streams with multiple sinks
  // to prevent them from consuming the values that each other expects.

  namespace detail {
    template <typename T>
    struct SharedSourceBinder {
      using value_type = T;

      std::shared_ptr<std::vector<push_fn<T>>> sinks;
      pull_fn pull;

      RHEOSCAPE_CALLABLE pull_fn operator()(push_fn<T> push) const {
        sinks->push_back(push);
        return pull;
      }
    };
  }

  template <typename SourceT>
    requires concepts::Source<SourceT>
  RHEOSCAPE_CALLABLE auto share(SourceT source) {
    using T = source_value_t<SourceT>;
    auto sinks = std::make_shared<std::vector<push_fn<T>>>();

    struct PushHandler {
      std::shared_ptr<std::vector<push_fn<T>>> sinks;

      RHEOSCAPE_CALLABLE void operator()(T value) const {
        for (push_fn<T> sink : *sinks) {
          sink(value);
        }
      }
    };

    pull_fn pull = source(PushHandler{sinks});

    return detail::SharedSourceBinder<T>{sinks, std::move(pull)};
  }

  namespace detail {
    struct SharePipeFactory {
      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return share(std::move(source));
      }
    };
  }

  inline auto share() {
    return detail::SharePipeFactory{};
  }

}
