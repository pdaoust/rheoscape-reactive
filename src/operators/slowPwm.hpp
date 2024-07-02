#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/zip.hpp>

namespace rheo {

  // Given a clock and a time interval,
  // turn a float between 0.0 and 1.0 into a slow PWM
  // that outputs `on` in the first part of the cycle
  // and `off` in the second.
  // The cycle is aligned with the clock's epoch start.

  template <typename TTimePoint, typename TInterval>
  source_fn<SwitchState> slowPwm(source_fn<float> dutySource, source_fn<TTimePoint> clockSource, TInterval cycleLength) {
    auto zipped = zip<std::tuple<float, TTimePoint>>(dutySource, clockSource);
    return map(
      zipped,
      (map_fn<SwitchState>)[cycleLength](std::tuple<float, TTimePoint> value) {
        return (std::get<1>(value) % cycleLength) < std::get<0>(value) * cycleLength
          ? SwitchState::on
          : SwitchState::off;
      }
    );
  }

  template <typename TTimePoint, typename TInterval>
  pipe_fn<float, SwitchState> slowPwm(source_fn<TTimePoint> clockSource, TInterval cycleLength) {
    return [clockSource, cycleLength](source_fn<float> dutySource) {
      return slowPwm(dutySource, clockSource, cycleLength);
    };
  }

}