#ifndef RHEOSCAPE_ARDUINO_MILLIS_CLOCK
#define RHEOSCAPE_ARDUINO_MILLIS_CLOCK

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

class arduino_millis_clock {
  public:
    typedef unsigned long ref;
    typedef std::milli period;
    typedef std::chrono::duration<unsigned long, std::milli> duration;
    typedef std::chrono::time_point<arduino_millis_clock, arduino_millis_clock::duration> time_point;

    static constexpr bool is_steady = true;

    static arduino_millis_clock::time_point now() {
      return arduino_millis_clock::time_point(arduino_millis_clock::duration(millis()));
    }
};

source_fn<arduino_millis_clock::time_point> arduinoMillisClock() {
  return [](push_fn<arduino_millis_clock::time_point> push) {
    return [push]() {
      push(arduino_millis_clock::now());
    };
  };
}

#endif