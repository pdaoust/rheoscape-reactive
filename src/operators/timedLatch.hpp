#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  // When the passed source changes,
  // hold a value for a while before reverting to a default value.
  // The initial non-default value will be pushed,
  // then the default value will be pushed at the end of the latch duration
  // (unless upstream has some new value, in which case it'll latch to that instead).
  // Of course you can pull it as many times as you like inside or out of a latch
  // to get the latched or default value too.
  // If the value changes to the default or another value
  // before the latch period is up,
  // you'll still get the value that triggered the latch period
  // rather than whatever it is now.
  //
  // This source ends when either of its sources ends.

  // TODO: I'm too tired right now to figure out
  // whether this could be done with higher-order operators.
  // Investigate whether it can!
  template <typename T, typename TTimePoint, typename TInterval>
  source_fn<T> timedLatch(source_fn<T> source, source_fn<TTimePoint> clockSource, TInterval duration, T defaultValue) {
    return [source, clockSource, duration, defaultValue](push_fn<T> push) {
      auto lastTimestamp = std::make_shared<std::optional<TTimePoint>>();
      auto latchStartTimestamp = std::make_shared<std::optional<TTimePoint>>();

      pull_fn pullClock = clockSource([lastTimestamp](TTimePoint ts) {
        lastTimestamp->emplace(ts);
      });

      pull_fn pullSource = source([duration, defaultValue, push, lastTimestamp, latchStartTimestamp](T value) {
        if (value == defaultValue) {
          push(value);
        } else if (!latchStartTimestamp->has_value()) {
          // Starting a new latch period.
          latchStartTimestamp->emplace(lastTimestamp->value());
          push(value);
        } else if (lastTimestamp->value() - latchStartTimestamp->value() < duration) {
          // Within the latch period.
          push(value);
        } else {
          // Outside the latch period.
          // End it.
          latchStartTimestamp->reset();
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
      return timedLatch(source, clockSource, duration, defaultValue);
    };
  }

}