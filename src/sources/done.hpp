#pragma once

#include <types/core_types.hpp>
#include <types/Endable.hpp>

namespace rheoscape::sources {

  // A source function that ends immediately.

  namespace detail {

    template <typename T, typename PushFn>
    struct done_pull_handler {
      PushFn push;
      mutable bool is_done = false;

      RHEOSCAPE_CALLABLE void operator()() const {
        if (!is_done) {
          push(Endable<T>());
          is_done = true;
        }
      }
    };

    template <typename T>
    struct done_source_binder {
      using value_type = Endable<T>;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        return done_pull_handler<T, PushFn>{std::move(push)};
      }
    };

  } // namespace detail

  template <typename T>
  auto done() {
    return detail::done_source_binder<T>{};
  }

}
