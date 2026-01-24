#pragma once

#include <core_types.hpp>
#include <Endable.hpp>

namespace rheo::sources {

  // A source function that ends immediately.

  template <typename T>
  struct done_pull_handler {
    push_fn<Endable<T>> push;
    mutable bool is_done = false;

    RHEO_CALLABLE void operator()() const {
      if (!is_done) {
        push(Endable<T>());
        is_done = true;
      }
    }
  };

  template <typename T>
  struct done_source_binder {
    RHEO_CALLABLE pull_fn operator()(push_fn<Endable<T>> push) const {
      return done_pull_handler<T>{std::move(push)};
    }
  };

  template <typename T>
  source_fn<Endable<T>> done() {
    return done_source_binder<T>{};
  }

}