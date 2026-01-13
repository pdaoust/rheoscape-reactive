#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::sources {

  template <typename TClock>
  struct from_clock_pull_handler {
    push_fn<typename TClock::time_point> push;

    RHEO_NOINLINE void operator()() const {
      push(TClock::now());
    }
  };

  template <typename TClock>
  struct from_clock_source_binder {
    RHEO_NOINLINE pull_fn operator()(push_fn<typename TClock::time_point> push) const {
      return from_clock_pull_handler<TClock>{std::move(push)};
    }
  };

  template <typename TClock>
  source_fn<typename TClock::time_point> from_clock() {
    return from_clock_source_binder<TClock>{};
  }

  template <typename TClock>
  map_fn<typename TClock::duration, typename TClock::time_point> to_duration = [](typename TClock::time_point value) { return value.time_since_epoch(); };

  template <typename TClock>
  map_fn<typename TClock::duration::rep, typename TClock::time_point> to_rep = [](typename TClock::time_point value) { return value.time_since_epoch().count(); };

}