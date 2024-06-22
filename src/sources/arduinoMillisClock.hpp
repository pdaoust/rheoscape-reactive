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

source_fn<unsigned long> arduinoMillisClock() {
  return [](push_fn<unsigned long> push) {
    return [push]() {
      push(millis());
    };
  };
}

#endif