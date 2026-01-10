#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::sources {

  // Pull handler that does nothing - pulling from an empty source has no effect
  struct empty_pull_handler {
    RHEO_NOINLINE void operator()() const {
      // Intentionally empty
    }
  };

  // Source binder that ignores the push function and returns an empty pull handler
  template <typename T>
  struct empty_source_binder {
    RHEO_NOINLINE pull_fn operator()(push_fn<T>) const {
      return empty_pull_handler{};
    }
  };

  template <typename T>
  source_fn<T> empty() {
    return empty_source_binder<T>{};
  }

  template <typename T>
  pull_fn empty(push_fn<T>) {
    return empty_pull_handler{};
  }

}