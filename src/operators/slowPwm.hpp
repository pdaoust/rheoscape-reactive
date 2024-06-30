#ifndef RHEOSCAPE_SLOW_PWM
#define RHEOSCAPE_SLOW_PWM

#include <functional>
#include <core_types.hpp>
#include <operators/zip.hpp>

// Given a clock and a time interval,
// turn a float between 0.0 and 1.0 into a slow PWM
// that outputs true in the first part of the cycle
// and false in the second.
// The cycle is aligned with the clock's epoch start.
template <typename TTimePoint, typename TInterval>
source_fn<bool> slowPwm_(source_fn<float> dutySource, source_fn<TTimePoint> clockSource, TInterval cycleLength) {
  auto zipped = zip_<std::tuple<float, TTimePoint>>(dutySource, clockSource);
  return map_(
    zipped,
    (map_fn<bool>)[cycleLength](std::tuple<float, TTimePoint> value) {
      return (std::get<1>(value) % cycleLength) < std::get<0>(value) * cycleLength; 
    }
  );
}

template <typename TTimePoint, typename TInterval>
pipe_fn<float, bool> slowPwm(source_fn<TTimePoint> clockSource, TInterval cycleLength) {
  return [clockSource, cycleLength](source_fn<float> dutySource) {
    return slowPwm_(dutySource, clockSource, cycleLength);
  };
}

#endif