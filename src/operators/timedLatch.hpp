#ifndef RHEOSCAPE_TIMED_LATCH
#define RHEOSCAPE_TIMED_LATCH

#include <functional>
#include <core_types.hpp>

// When the passed source changes,
// hold a value for a while before reverting to a default value.
// The initial non-default value will be pushed,
// then the default value will be pushed at the end of the latch duration.
// Of course you can push it as many times as you like inside or out of a latch
// to get the latched or default value too.
// If the value changes to the default or another value
// before the latch period is up,
// you'll still get the value that triggered the latch period
// rather than whatever it is now.
template <typename T, typename TTimePoint, typename TInterval>
source_fn<T> timedLatch_(source_fn<T> source, source_fn<TTimePoint> clockSource, TInterval duration, T defaultValue) {
  return [source, clockSource, duration, defaultValue](push_fn<T> push) {
    std::optional<T> lastTimestamp;
    std::optional<TTimePoint> latchStartTimestamp;

    pull_fn pullClock = clockSource([&lastTimestamp](TTimePoint ts) {
      lastTimestamp = ts;
    });

    pull_fn pullSource = source([duration, defaultValue, push, lastTimestamp, &latchStartTimestamp](T value) {
      if (value == defaultValue) {
        push(value);
      } else if (!latchStartTimestamp.has_value()) {
        // Starting a new latch period.
        latchStartTimestamp = lastTimestamp;
        push(value);
      } else if (lastTimestamp.value() - latchStartTimestamp.value() < duration) {
        // Within the latch period.
        push(value);
      } else {
        // Outside the latch period.
        // End it.
        latchStartTimestamp = std::nullopt;
        push(defaultValue);
      }
    });

    return [pullClock, pullSource]() {
      pullClock();
      pullSource();
    };
  };
}

template <typename T, typename TTimePoint, typename TInterval>
pipe_fn<T, T> timedLatch(source_fn<TTimePoint> clockSource, TInterval duration, T defaultValue) {
  return [clockSource, duration, defaultValue](source_fn<T> source) {
    return timedLatch_(source, clockSource, duration, defaultValue);
  };
}

#endif