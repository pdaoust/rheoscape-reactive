#pragma once

#include <array>
#include <functional>
#include <core_types.hpp>

namespace rheo::sources {

  namespace detail {

    // Adapter that forwards values from an observable to a push function
    template <typename T, typename PushFn>
    struct from_observable_push_adapter {
      PushFn push;

      RHEO_CALLABLE void operator()(T value) const {
        push(std::move(value));
      }
    };

    // Pull handler that does nothing - pulling from an observable is meaningless
    struct from_observable_empty_pull {
      RHEO_CALLABLE void operator()() const {
        // It doesn't mean anything to pull from an observable.
      }
    };

    template <typename SubscribeFn, typename T>
    struct from_observable_source_binder {
      using value_type = T;
      SubscribeFn subscribe_fn;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {
        subscribe_fn(from_observable_push_adapter<T, PushFn>{std::move(push)});
        return from_observable_empty_pull{};
      }
    };

  } // namespace detail

  template <typename SubscribeFn>
  auto from_observable(SubscribeFn&& subscribe_fn)
  -> source_fn<source_value_type_t<std::decay_t<SubscribeFn>>> {
    using T = source_value_type_t<std::decay_t<SubscribeFn>>;
    return detail::from_observable_source_binder<std::decay_t<SubscribeFn>, T>{
      std::forward<SubscribeFn>(subscribe_fn)
    };
  }

}
