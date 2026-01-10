#pragma once

#include <core_types.hpp>
#include <Endable.hpp>

namespace rheo::sources {

  // A source function that ends immediately.

  template <typename T>
  struct done_pull_handler {
    push_fn<Endable<T>> push;
    mutable bool isDone = false;

    RHEO_NOINLINE void operator()() const {
      if (!isDone) {
        push(Endable<T>());
        isDone = true;
      }
    }
  };

  template <typename T>
  struct done_source_binder {
    RHEO_NOINLINE pull_fn operator()(push_fn<Endable<T>> push) const {
      return done_pull_handler<T>{std::move(push)};
    }
  };

  template <typename T>
  source_fn<Endable<T>> done() {
    return done_source_binder<T>{};
  }

}