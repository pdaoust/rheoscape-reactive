#pragma once

#include <functional>
#include <memory>
#include <core_types.hpp>

namespace rheo::operators {

  // When the passed source changes,
  // hold a value for a while before reverting to a default value.
  // The initial non-default value will be pushed,
  // then the default value will be pushed at the end of the latch duration
  // (unless upstream has some new value, in which case it'll latch to that instead --
  // and if the new value is the same as the value that was just latched,
  // then it'll stay latched to that for another latch duration).
  // Of course you can pull it as many times as you like inside or out of a latch
  // to get the latched or default value too.
  // If the value changes to the default or another value
  // before the latch period is up,
  // you'll still get the value that triggered the latch period
  // rather than whatever it is now.

  template <typename TTimePoint>
  struct timedLatch_clock_push_handler {
    std::shared_ptr<std::optional<TTimePoint>> lastTimestamp;

    RHEO_NOINLINE void operator()(TTimePoint ts) const {
      lastTimestamp->emplace(ts);
    }
  };

  template <typename T, typename TTimePoint, typename TInterval>
  struct timedLatch_push_handler {
    TInterval duration;
    T defaultValue;
    push_fn<T> push;
    std::shared_ptr<std::optional<TTimePoint>> lastTimestamp;
    pull_fn pullClock;
    std::shared_ptr<std::optional<TTimePoint>> latchStartTimestamp;
    std::shared_ptr<std::optional<T>> lastValue;

    RHEO_NOINLINE void operator()(T value) const {
      pullClock();

      if (latchStartTimestamp->has_value()) {
        if (lastTimestamp->value() - latchStartTimestamp->value() < duration) {
          // Within the latch period; no changes.
          push(lastValue->value());
          return;
        }

        // Expired; reset latch counter.
        latchStartTimestamp->reset();
      }

      // Either the latch duration has expired or the latch has never been triggered.
      // If it's a non-default value, start a new latch interval.
      if (value != defaultValue) {
        latchStartTimestamp->emplace(lastTimestamp->value());
        lastValue->emplace(value);
      }

      push(value);
    }
  };

  template <typename T, typename TTimePoint, typename TInterval>
  struct timedLatch_source_binder {
    source_fn<T> source;
    source_fn<TTimePoint> clockSource;
    TInterval duration;
    T defaultValue;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      auto lastTimestamp = std::make_shared<std::optional<TTimePoint>>();

      // Rather than use the combine() operator,
      // which would pull a T value every time the clock pushes,
      // we only pull the clock when a new T is pushed.
      // So first we bind to the clock source here,
      // then we pull from the clock source in the push function.
      pull_fn pullClock = clockSource(timedLatch_clock_push_handler<TTimePoint>{lastTimestamp});

      // Shared pointers needed to share state between bindings.
      auto latchStartTimestamp = std::make_shared<std::optional<TTimePoint>>();
      auto lastValue = std::make_shared<std::optional<T>>();

      return source(timedLatch_push_handler<T, TTimePoint, TInterval>{
        duration,
        defaultValue,
        std::move(push),
        lastTimestamp,
        std::move(pullClock),
        latchStartTimestamp,
        lastValue
      });
    }
  };

  template <typename T, typename TTimePoint, typename TInterval>
  source_fn<T> timedLatch(source_fn<T> source, source_fn<TTimePoint> clockSource, TInterval duration, T defaultValue) {
    return timedLatch_source_binder<T, TTimePoint, TInterval>{
      std::move(source),
      std::move(clockSource),
      duration,
      std::move(defaultValue)
    };
  }

  template <typename T, typename TTimePoint, typename TInterval>
  pipe_fn<T, T> timedLatch(source_fn<TTimePoint> clockSource, TInterval duration, T defaultValue) {
    return [clockSource, duration, defaultValue](source_fn<T> source) {
      return timedLatch(source, clockSource, duration, defaultValue);
    };
  }

}