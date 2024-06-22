#ifndef RHEOSCAPE_SLOW_PWM
#define RHEOSCAPE_SLOW_PWM

#include <functional>
#include <core_types.hpp>

template <typename TTimePoint, typename TInterval>
source_fn<bool> slowPwm_(source_fn<float> dutySource, source_fn<TTimePoint> clockSource, TInterval cycleLength) {
  return [dutySource, clockSource, cycleLength](push_fn<bool> push) {
    std::optional<TTimePoint> lastCycleStart;
    std::optional<TInterval> elapsed;

    pull_fn pullClock = clockSource([&lastCycleStart, &elapsed, cycleLength](TTimePoint ts) {
      elapsed = ts - lastCycleStart;
      if (elapsed >= cycleLength) {
        // Catch up if it's been more than one cycle since we sampled the clock.
        lastCycleStart += cycleLength * floor(elapsed / cycleLength);
      }
    });

    pull_fn pullDuty = dutySource([elapsed, cycleLength, push](float value) {
      push(value > 0 && elapsed / cycleLength <= value);
    });

    return []() {
      pullClock();
      pullDuty();
    };
  };
}

template <typename TTimePoint, typename TInterval>
pipe_fn<float, bool> slowPwm(source_fn<TTimePoint> clockSource, TInterval cycleLength) {
  return [clockSource, cycleLength](source_fn<float> dutySource) {
    return slowPwm_(dutySource, clockSource, cycleLength);
  };
}

#endif