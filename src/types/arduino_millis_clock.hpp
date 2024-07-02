#pragma once

#include <chrono>
#include <types/TSValue.hpp>
#include <Arduino.h>

namespace rheo {

  class arduino_millis_clock {
    public:
      typedef std::chrono::duration<unsigned long, std::milli> duration;
      typedef arduino_millis_clock::duration::rep rep;
      typedef arduino_millis_clock::duration::period period;
      typedef std::chrono::time_point<arduino_millis_clock, arduino_millis_clock::duration> time_point;

      static constexpr bool is_steady = true;

      static arduino_millis_clock::time_point now() noexcept {
        return arduino_millis_clock::time_point(arduino_millis_clock::duration(millis()));
      }
  };

  template <typename T>
  struct ATSValue : TSValue<arduino_millis_clock::time_point, T> {};

}