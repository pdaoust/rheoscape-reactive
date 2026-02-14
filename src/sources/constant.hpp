#pragma once

#include <core_types.hpp>

namespace rheo::sources {

  namespace detail {

    template <typename T, typename PushFn>
    struct constant_pull_handler {
      T value;
      PushFn push;

      RHEO_CALLABLE void operator()() const {
        push(value);
      }
    };

    template <typename T>
    struct constant_source_binder {
      using value_type = T;
      T value;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {
        return constant_pull_handler<T, PushFn>{value, std::move(push)};
      }
    };

  } // namespace detail

  template <typename T>
  auto constant(T value) {
    return detail::constant_source_binder<T>{std::move(value)};
  }

}
