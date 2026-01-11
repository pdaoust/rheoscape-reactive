#pragma once

#include <functional>
#include <core_types.hpp>
#include <util.hpp>
#include <sources/constant.hpp>

namespace rheo::operators {

  // Send a timestamp at the interval specified by the interval source.
  // The time source should supply its numbers regularly,
  // as time points that can be subtracted from each other
  // to yield values of the interval source's type.
  // (For Arduino millis, that's unsigned long for both.)
  // The interval source must be pullable,
  // must yield another number to specify the next interval
  // after the previous interval has passed,
  // and should probably not do its own pushing.
  // The simplest interval source is `constant(interval)`.
  // (There's an alias for it called `every`, in this file.)
  // Or, for exponential backoff, you could use the `curve` function.
  //
  // This function ends when either of its sources ends.

  // TODO: could this entire thing be replaced with a filter or a reduce?
  // Something like
  //    filter(timeSource, [](ts) { return !(ts % interval); }
  // But with something that would allow it to snap to time intervals if it missed one

  template <typename TInterval>
  struct interval_interval_push_handler {
    std::shared_ptr<std::optional<TInterval>> lastInterval;

    RHEO_NOINLINE void operator()(TInterval interval) const {
      lastInterval->emplace(interval);
    }
  };

  template <typename TTimePoint, typename TInterval>
  struct interval_time_push_handler {
    std::shared_ptr<std::optional<TInterval>> lastInterval;
    std::shared_ptr<std::optional<TTimePoint>> lastIntervalTimestamp;
    pull_fn pullNextInterval;
    push_fn<TTimePoint> push;

    RHEO_NOINLINE void operator()(TTimePoint timestamp) const {
      if (!lastInterval->has_value()) {
        pullNextInterval();
        if (!lastInterval->has_value()) {
          // Can't start yet; we have no interval.
          return;
        }
      }

      // This comes after we pull the first interval.
      // That's because we don't want to start counting
      // until we know what we're counting from/to.
      if (!lastIntervalTimestamp->has_value()) {
        // First pull or push of a timestamp; start the thing!
        lastIntervalTimestamp->emplace(timestamp);
      }

      if (timestamp - lastIntervalTimestamp->value() >= lastInterval->value()) {
        // An interval has passed. Push the timestamp and get the next interval.
        // Don't use the current timestamp; that'll result in uneven interval spacing!
        // Better to have unevenly emitted timestamps than unevenly calculated intervals.
        lastIntervalTimestamp->emplace(lastIntervalTimestamp->value() + lastInterval->value());
        push(lastIntervalTimestamp->value());
        pullNextInterval();
      }
    }
  };

  template <typename TTimePoint, typename TInterval>
  struct interval_source_binder {
    source_fn<TTimePoint> timeSource;
    source_fn<TInterval> intervalSource;

    RHEO_NOINLINE pull_fn operator()(push_fn<TTimePoint> push) const {
      auto lastInterval = std::make_shared<std::optional<TInterval>>();
      auto lastIntervalTimestamp = std::make_shared<std::optional<TTimePoint>>();

      pull_fn pullNextInterval = intervalSource(interval_interval_push_handler<TInterval>{lastInterval});

      return timeSource(interval_time_push_handler<TTimePoint, TInterval>{
        lastInterval,
        lastIntervalTimestamp,
        std::move(pullNextInterval),
        std::move(push)
      });
    }
  };

  template <typename TTimePoint, typename TInterval>
  source_fn<TTimePoint> interval(source_fn<TTimePoint> timeSource, source_fn<TInterval> intervalSource) {
    return interval_source_binder<TTimePoint, TInterval>{
      std::move(timeSource),
      std::move(intervalSource)
    };
  }

  template <typename TTimePoint, typename TInterval>
  pipe_fn<TTimePoint, TTimePoint> interval(source_fn<TInterval> intervalSource) {
    return [intervalSource](source_fn<TTimePoint> timeSource) {
      return interval(timeSource, intervalSource);
    };
  }

  // An alias for `constant`, to make it read better!
  template <typename TInterval>
  source_fn<TInterval> every(TInterval interval) {
    return sources::constant(interval);
  }

  // Useful for exponential backoff.
  // I wouldn't recommend using a factor <1,
  // cuz then it'd just start getting faster and faster!
  template <typename TInterval>
  struct curve_pull_handler {
    mutable TInterval acc;
    float factor;
    push_fn<TInterval> push;

    RHEO_NOINLINE void operator()() const {
      push(acc);
      acc *= factor;
    }
  };

  template <typename TInterval>
  struct curve_source_binder {
    TInterval startInterval;
    float factor;

    RHEO_NOINLINE pull_fn operator()(push_fn<TInterval> push) const {
      return curve_pull_handler<TInterval>{startInterval, factor, std::move(push)};
    }
  };

  template <typename TInterval>
  source_fn<TInterval> curve(TInterval startInterval, float factor) {
    return curve_source_binder<TInterval>{std::move(startInterval), factor};
  }

}