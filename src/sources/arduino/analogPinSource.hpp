#ifndef RHEOSCAPE_ANALOG_PIN_SOURCE
#define RHEOSCAPE_ANALOG_PIN_SOURCE

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

source_fn<float> analogPinSource(int pin, uint8_t resolutionBits = 10) {
  return [pin, resolutionBits](push_fn<float> push, end_fn _) {
    return [pin, resolutionBits, push]() {
      analogReadResolution(resolutionBits);
      push(analogRead(pin));
    };
  };
}

#endif