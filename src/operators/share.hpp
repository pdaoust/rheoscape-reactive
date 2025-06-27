#pragma once

#include <functional>
#include <fmt/format.h>
#include <core_types.hpp>
#include <logging.hpp>

namespace rheo::operators {

  // Sharing a stream is like tapping it,
  // except that when any sink pulls, the value gets pushed to all sinks.
  // This function is useful for sharing streams with multiple sinks
  // to prevent them from consuming the values that each other expects.
  template <typename T>
  source_fn<T> share(source_fn<T> source) {
    auto sinks = std::make_shared<std::vector<push_fn<T>>>();

    auto pull = source([sinks](T value) {
      for (push_fn<T> sink : *sinks) {
        sink(value);
      }
    });

    return [sinks, pull](push_fn<T> push) {
      sinks->push_back(push);
      return pull;
    };
  }

  template <typename T>
  pipe_fn<T, T> share() {
    return [](source_fn<T> source) {
      return share(source);
    };
  }

}