#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::sources {

  template <typename T>
  struct constant_pull_handler {
    T value;
    push_fn<T> push;

    RHEO_NOINLINE void operator()() const {
      push(value);
    }
  };

  template <typename T>
  struct constant_source_binder {
    T value;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      return constant_pull_handler<T>{value, std::move(push)};
    }
  };

  template <typename T>
  source_fn<T> constant(T value) {
    return constant_source_binder<T>{std::move(value)};
  }

}