#pragma once

#include <array>
#include <functional>
#include <core_types.hpp>

namespace rheo::sources {

  // Adapter that forwards values from an observable to a push function
  template <typename T>
  struct from_observable_push_adapter {
    push_fn<T> push;

    RHEO_NOINLINE void operator()(T value) const {
      push(std::move(value));
    }
  };

  // Pull handler that does nothing - pulling from an observable is meaningless
  struct from_observable_empty_pull {
    RHEO_NOINLINE void operator()() const {
      // It doesn't mean anything to pull from an observable.
    }
  };

  template <typename SubscribeFn, typename T>
  struct from_observable_source_binder {
    SubscribeFn subscribe_fn;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      subscribe_fn(from_observable_push_adapter<T>{std::move(push)});
      return from_observable_empty_pull{};
    }
  };

  template <typename SubscribeFn>
  auto from_observable(SubscribeFn&& subscribe_fn)
  -> source_fn<source_value_type_t<std::decay_t<SubscribeFn>>> {
    using T = source_value_type_t<std::decay_t<SubscribeFn>>;
    return from_observable_source_binder<std::decay_t<SubscribeFn>, T>{
      std::forward<SubscribeFn>(subscribe_fn)
    };
  }

}