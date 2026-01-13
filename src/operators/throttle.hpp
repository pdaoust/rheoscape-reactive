#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/timestamp.hpp>

namespace rheo::operators {

  // Only allow at most one value per interval,
  // dropping everything in between.
  // Not guaranteed to be precisely spaced by the interval.

  template <typename T, typename TTime, typename TInterval>
  struct throttle_push_handler {
    TInterval interval;
    push_fn<T> push;
    mutable std::optional<TTime> interval_start;

    RHEO_NOINLINE void operator()(TaggedValue<T, TTime> value) const {
      if (interval_start.has_value() && value.tag - interval_start.value() > interval) {
        interval_start = std::nullopt;
      }

      if (!interval_start.has_value()) {
        interval_start = value.tag;
        push(value.value);
      }
    }
  };

  template <typename T, typename TTime, typename TInterval>
  struct throttle_source_binder {
    source_fn<TaggedValue<T, TTime>> timestamped;
    TInterval interval;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      return timestamped(throttle_push_handler<T, TTime, TInterval>{
        interval,
        std::move(push),
        std::nullopt
      });
    }
  };

  template <typename T, typename TTime, typename TInterval>
  source_fn<T> throttle(source_fn<T> source, source_fn<TTime> clock_source, TInterval interval) {
    return throttle_source_binder<T, TTime, TInterval>{
      timestamp(source, clock_source),
      interval
    };
  }

  template <typename T, typename TTime, typename TInterval>
  pipe_fn<T, T> throttle(source_fn<TTime> clock_source, TInterval interval) {
    return [clock_source, interval](source_fn<T> source) {
      return throttle(source, clock_source, interval);
    };
  }

}