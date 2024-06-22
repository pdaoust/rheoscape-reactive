#ifndef RHEOSCAPE_RANGE
#define RHEOSCAPE_RANGE

template <typename T>
struct Range {
  T min;
  T max;

  Range(T min, T max) : min(min), max(max) { }
};

template <typename T>
struct SetpointAndHysteresis {
  T setpoint;
  // The distance that the measured value may diverge from the setpoint in either direction.
  T hysteresis;

  SetpointAndHysteresis(T setpoint, T hysteresis) : setpoint(setpoint), hysteresis(hysteresis) { }

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

#endif