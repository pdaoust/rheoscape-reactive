#ifndef RHEOSCAPE_INTERVAL
#define RHEOSCAPE_INTERVAL

#include <functional>
#include <core_types.hpp>
#include <sources/constant.hpp>

// Send a timestamp at the interval specified by the interval source.
// The time source should supply its numbers regularly,
// as time points that can be subtracted from each other
// to yield values in the interval source.
// (For Arduino millis, that's unsigned long for both.)
// The interval source must be pullable,
// must yield a new number to specify the next interval
// after the previous interval has passed,
// and should probably not do its own pushing.
// The simplest interval source is `constant(interval)`.
// (There's an alias for it called `every`, in this file.)
// Or, for exponential backoff, you could use the `curve` function.

template <typename TTimePoint, typename TInterval>
source_fn<TTimePoint> interval_(source_fn<TTimePoint> timeSource, source_fn<TInterval> intervalSource) {
  return [timeSource, intervalSource](push_fn<TTimePoint> push) {
    std::optional<TInterval> lastInterval;
    std::optional<TTimePoint> lastIntervalTimestamp;

    pull_fn pullNextInterval = intervalSource([&lastInterval](TInterval interval) {
      lastInterval = interval;
    });

    return timeSource([&lastInterval, &lastIntervalTimestamp, pullNextInterval, push](TTimePoint timestamp) {
      if (!lastIntervalTimestamp.has_value()) {
        // First pull or push of a timestamp; start the thing!
        lastIntervalTimestamp = timestamp;
      }

      if (!lastInterval.has_value() || timestamp - lastIntervalTimestamp.value() >= lastInterval.value()) {
        // An interval has passed. Push the timestamp and get the next interval.
        // Don't use the current timestamp; that'll result in uneven interval spacing!
        // Better to have unevenly emitted timestamps than unevenly calculated intervals.
        lastIntervalTimestamp = lastIntervalTimestamp.value() + lastInterval.value();
        push(lastIntervalTimestamp.value());
        pullNextInterval();
      }
    });
  };
}

template <typename TTimePoint, typename TInterval>
pipe_fn<TTimePoint, TTimePoint> interval(source_fn<TInterval> intervalSource) {
  return [intervalSource](source_fn<TTimePoint> timeSource) {
    return interval_(timeSource, intervalSource);
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

#endif