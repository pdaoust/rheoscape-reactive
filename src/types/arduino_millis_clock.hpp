#pragma once

#include <chrono>
#include <Arduino.h>

namespace rheo {
  using arduino_millis_clock_duration = std::chrono::duration<unsigned long, std::milli>;

  class arduino_millis_clock {
    public:
      typedef arduino_millis_clock_duration duration;
      typedef unsigned long rep;
      typedef std::milli period;
      typedef std::chrono::time_point<arduino_millis_clock, arduino_millis_clock::duration> time_point;

      static constexpr bool is_steady = true;

      static arduino_millis_clock::time_point now() noexcept {
        return arduino_millis_clock::time_point(arduino_millis_clock::duration(millis()));
      }
  };

  using arduino_millis_clock_time_point = std::chrono::time_point<arduino_millis_clock, arduino_millis_clock_duration>;

}

template <>
struct fmt::formatter<rheo::arduino_millis_clock_duration> : fmt::formatter<unsigned long> {
  auto format(const rheo::arduino_millis_clock_duration& ts, format_context& ctx) const {
    return fmt::formatter<unsigned long>::format(ts.count(), ctx);
  }
};

template <>
struct fmt::formatter<rheo::arduino_millis_clock_time_point> : fmt::formatter<unsigned long> {
  auto format(const rheo::arduino_millis_clock_time_point& ts, format_context& ctx) const {
    return fmt::formatter<unsigned long>::format(ts.time_since_epoch().count(), ctx);
  }
};
