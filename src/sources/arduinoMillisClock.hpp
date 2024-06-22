#ifndef RHEOSCAPE_ARDUINO_MILLIS_CLOCK
#define RHEOSCAPE_ARDUINO_MILLIS_CLOCK

#include <functional>
#include <core_types.hpp>
#include <operators/castUnit.hpp>
#include <operators/map.hpp>
#include <Arduino.h>

using arduino_ms = au::Quantity<au::Milli<au::Seconds>, unsigned long>;
using arduino_ms_float = au::Quantity<au::Milli<au::Seconds>, float>;
constexpr auto make_arduino_ms = au::milli(au::seconds);


source_fn<arduino_ms> arduinoMillisClock() {
  return [](push_fn<arduino_ms> push) {
    return [push]() {
      push(make_arduino_ms(millis()));
    };
  };
}

source_fn<arduino_ms_float> arduinoMillisClockFloat() {
  return map_<arduino_ms, arduino_ms_float>(arduinoMillisClock(), [](arduino_ms value) { return au::rep_cast<float>(value); });
}

#endif