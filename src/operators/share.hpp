#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <core_types.hpp>

namespace rheo::operators {

  // Sharing a stream is like tapping it,
  // except that when any sink pulls, the value gets pushed to all sinks.
  // This function is useful for sharing streams with multiple sinks
  // to prevent them from consuming the values that each other expects.

  template <typename T>
  RHEO_CALLABLE source_fn<T> share(source_fn<T> source) {
    auto sinks = std::make_shared<std::vector<push_fn<T>>>();

    struct PushHandler {
      std::shared_ptr<std::vector<push_fn<T>>> sinks;

      RHEO_CALLABLE void operator()(T value) const {
        for (push_fn<T> sink : *sinks) {
          sink(value);
        }
      }
    };

    auto pull = source(PushHandler{sinks});

    struct SourceBinder {
      std::shared_ptr<std::vector<push_fn<T>>> sinks;
      pull_fn pull;

      RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {
        sinks->push_back(push);
        return pull;
      }
    };

    return SourceBinder{sinks, pull};
  }

  namespace detail {
    struct SharePipeFactory {
      template <typename T>
      RHEO_CALLABLE auto operator()(source_fn<T> source) const {
        return share(std::move(source));
      }
    };
  }

  inline auto share() {
    return detail::SharePipeFactory{};
  }

}
