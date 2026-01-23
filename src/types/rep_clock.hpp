#pragma once

#include <chrono>
#include <ratio>

namespace rheo {

  // A clock type parameterized by representation type, period, and steadiness.
  // This clock has no actual time source (now() is not implemented),
  // but serves as a target type for converting durations and time_points
  // from other clocks to a different representation.
  //
  // Example usage:
  //   using float_seconds_clock = rep_clock<float, std::ratio<1>>;
  //   using double_millis_clock = rep_clock<double, std::milli>;
  //
  // Then use with time_point_to_other_clock or duration_to_other_clock:
  //   auto float_tp = time_point_to_other_clock<float_seconds_clock, arduino_millis_clock>(millis_tp);
  template <typename TRep, typename TPeriod = std::ratio<1>, bool IsSteady = true>
  struct rep_clock {
    using rep = TRep;
    using period = TPeriod;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<rep_clock, duration>;

    static constexpr bool is_steady = IsSteady;

    // now() is intentionally not implemented.
    // This clock exists only for type conversions.
  };

  // Convenience aliases for common floating-point clocks.
  using float_seconds_clock = rep_clock<float, std::ratio<1>>;
  using float_millis_clock = rep_clock<float, std::milli>;
  using float_micros_clock = rep_clock<float, std::micro>;

  using double_seconds_clock = rep_clock<double, std::ratio<1>>;
  using double_millis_clock = rep_clock<double, std::milli>;
  using double_micros_clock = rep_clock<double, std::micro>;

}
