#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo {

  // When the passed source changes,
  // hold a value for a while before reverting to a default value.
  // The initial non-default value will be pushed,
  // then the default value will be pushed at the end of the latch duration.
  // Of course you can pull it as many times as you like inside or out of a latch
  // to get the latched or default value too.
  // If the value changes to the default or another value
  // before the latch period is up,
  // you'll still get the value that triggered the latch period
  // rather than whatever it is now.
  //
  // This source ends when either of its sources ends.

  // FIXME: I'm too tired right now to figure out
  // whether this could be done with higher-order operators.
  // Investigate whether it can!
  template <typename T, typename TTimePoint, typename TInterval>
  source_fn<T> timedLatch(source_fn<T> source, source_fn<TTimePoint> clockSource, TInterval duration, T defaultValue) {
    return [source, clockSource, duration, defaultValue](push_fn<T> push, end_fn end) {
      auto lastTimestamp = std::make_shared<std::optional<TTimePoint>>();
      auto latchStartTimestamp = std::make_shared<std::optional<TTimePoint>>();
      auto endAny = std::make_shared<EndAny>(end);

      pull_fn pullClock = clockSource(
        [lastTimestamp, endAny](TTimePoint ts) {
          if (endAny->ended) {
            return;
          }
          lastTimestamp->emplace(ts);
        },
        endAny->upstream_end_fn
      );

      pull_fn pullSource = source(
        [duration, defaultValue, push, lastTimestamp, latchStartTimestamp, endAny](T value) {
          if (endAny->ended) {
            return;
          }
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
        },
        endAny->upstream_end_fn
      );

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