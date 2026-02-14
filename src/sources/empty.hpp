#pragma once

#include <core_types.hpp>

namespace rheo::sources {

  namespace detail {

    // Pull handler that does nothing - pulling from an empty source has no effect
    struct empty_pull_handler {
      RHEO_CALLABLE void operator()() const {
        // Intentionally empty
      }
    };

    // Source binder that ignores the push function and returns an empty pull handler
    template <typename T>
    struct empty_source_binder {
      using value_type = T;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn) const {
        return empty_pull_handler{};
      }
    };

  } // namespace detail

  template <typename T>
  source_fn<T> empty() {
    return detail::empty_source_binder<T>{};
  }

  template <typename T>
  pull_fn empty(push_fn<T>) {
    return detail::empty_pull_handler{};
  }

}
