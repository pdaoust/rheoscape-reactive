#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/timestamp.hpp>

namespace rheo::operators {

  // Only allow at most one value per interval,
  // dropping everything in between.
  // Not guaranteed to be precisely spaced by the interval.

  template <typename T, typename TTimePoint, typename TInterval>
    requires concepts::TimePointAndDurationCompatible<TTimePoint, TInterval>
  struct throttle_push_handler {
    TInterval interval;
    push_fn<T> push;
    mutable std::optional<TTimePoint> interval_start;

    RHEO_NOINLINE void operator()(TaggedValue<T, TTimePoint> value) const {
      if (interval_start.has_value() && value.tag - interval_start.value() > interval) {
        interval_start = std::nullopt;
      }

      if (!interval_start.has_value()) {
        interval_start = value.tag;
        push(value.value);
      }
    }
  };

  template <typename T, typename TTimePoint, typename TInterval>
    requires concepts::TimePointAndDurationCompatible<TTimePoint, TInterval>
  struct throttle_source_binder {
    source_fn<TaggedValue<T, TTimePoint>> timestamped;
    TInterval interval;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      return timestamped(throttle_push_handler<T, TTimePoint, TInterval>{
        interval,
        std::move(push),
        std::nullopt
      });
    }
  };

  template <typename T, typename TTimePoint, typename TInterval>
    requires concepts::TimePointAndDurationCompatible<TTimePoint, TInterval>
  source_fn<T> throttle(source_fn<T> source, source_fn<TTimePoint> clock_source, TInterval interval) {
    return throttle_source_binder<T, TTimePoint, TInterval>{
      timestamp(source, clock_source),
      interval
    };
  }

  template <typename T, typename TTimePoint, typename TInterval>
    requires concepts::TimePointAndDurationCompatible<TTimePoint, TInterval>
  pipe_fn<T, T> throttle(source_fn<TTimePoint> clock_source, TInterval interval) {
    return [clock_source, interval](source_fn<T> source) {
      return throttle(source, clock_source, interval);
    };
  }

}