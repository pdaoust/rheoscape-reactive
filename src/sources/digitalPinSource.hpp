#ifndef RHEOSCAPE_DIGITAL_PIN_SOURCE
#define RHEOSCAPE_DIGITAL_PIN_SOURCE

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

source_fn<bool> digitalPinSource(int pin) {
  return [pin](push_fn<bool> push) {
    return [pin, push]() {
      pinMode(pin, INPUT);
      push((bool)digitalRead(pin));
    };
  };
}

#endif