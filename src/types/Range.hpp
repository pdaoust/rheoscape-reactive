#pragma once

namespace rheo {

  template <typename T>
  struct Range {
    const T min;
    const T max;

    Range(const T min, const T max) : min(min), max(max) { }
  };

  template <typename T>
  struct SetpointAndHysteresis {
    const T setpoint;
    // The distance that the measured value may diverge from the setpoint in either direction.
    const T hysteresis;

    SetpointAndHysteresis(const T setpoint, const T hysteresis) : setpoint(setpoint), hysteresis(hysteresis) { }

    SetpointAndHysteresis(Range<T> range) 
    :
      setpoint(range.min + (range.max - range.min) / 2),
      hysteresis((range.max - range.min) / 2)
    { }

    operator Range<T>() const {
      return Range<T>(min(), max());
    }

    T min() const {
      return setpoint - hysteresis;
    }

    T max() const {
      return setpoint + hysteresis;
    }
  };

}