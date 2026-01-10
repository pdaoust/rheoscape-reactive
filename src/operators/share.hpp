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

  // Named callable for share's push handler
  template<typename T>
  struct share_push_handler {
    std::shared_ptr<std::vector<push_fn<T>>> sinks;

    RHEO_NOINLINE void operator()(T value) const {
      for (push_fn<T> sink : *sinks) {
        sink(value);
      }
    }
  };

  // Named callable for share's source binder
  template<typename T>
  struct share_source_binder {
    std::shared_ptr<std::vector<push_fn<T>>> sinks;
    pull_fn pull;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      sinks->push_back(push);
      return pull;
    }
  };

  template <typename T>
  RHEO_INLINE source_fn<T> share(source_fn<T> source) {
    auto sinks = std::make_shared<std::vector<push_fn<T>>>();
    auto pull = source(share_push_handler<T>{sinks});

    return share_source_binder<T>{sinks, pull};
  }

  template <typename T>
  pipe_fn<T, T> share() {
    return [](source_fn<T> source) {
      return share(source);
    };
  }

}