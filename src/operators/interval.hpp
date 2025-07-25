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

  template <typename TTimePoint, typename TInterval>
  source_fn<TTimePoint> interval(source_fn<TTimePoint> timeSource, source_fn<TInterval> intervalSource) {
    return [timeSource, intervalSource](push_fn<TTimePoint> push) {
      auto lastInterval = std::make_shared<std::optional<TInterval>>();
      auto lastIntervalTimestamp = std::make_shared<std::optional<TTimePoint>>();

      pull_fn pullNextInterval = intervalSource([lastInterval](TInterval interval) {
        lastInterval->emplace(interval);
      });

      return timeSource([lastInterval, lastIntervalTimestamp, pullNextInterval, push](TTimePoint timestamp) {
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
      });
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
    return constant(interval);
  }

  // Useful for exponential backoff.
  // I wouldn't recommend using a factor <1,
  // cuz then it'd just start getting faster and faster!
  template <typename TInterval>
  source_fn<TInterval> curve(TInterval startInterval, float factor) {
    return [startInterval, factor](push_fn<TInterval> push) {
      TInterval acc = startInterval;
      return [factor, &acc, push] {
        push(acc);
        acc *= factor;
      };
    };
  }

}