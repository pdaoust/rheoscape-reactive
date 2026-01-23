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
  map_fn<typename TClock::duration::rep, typename TClock::duration> duration_to_rep = [](typename TClock::duration value) { return value.count(); };

  template <typename TClock>
  map_fn<typename TClock::duration::rep, typename TClock::time_point> time_point_to_rep = [](typename TClock::time_point value) { return value.time_since_epoch().count(); };

  // Convert a duration from one clock to another clock's duration type.
  template <typename TTargetClock, typename TSourceClock>
  auto duration_to_clock = [](typename TSourceClock::duration duration) -> typename TTargetClock::duration {
    return std::chrono::duration_cast<typename TTargetClock::duration>(duration);
  };

  // Convert a time_point from one clock to another clock's time_point type.
  // Note: This assumes both clocks share the same epoch, which is true for steady_clock variants
  // but may not be meaningful for clocks with different epochs.
  template <typename TTargetClock, typename TSourceClock>
  auto time_point_to_clock = [](typename TSourceClock::time_point time_point) -> typename TTargetClock::time_point {
    auto target_duration = std::chrono::duration_cast<typename TTargetClock::duration>(time_point.time_since_epoch());
    return typename TTargetClock::time_point(target_duration);
  };

}