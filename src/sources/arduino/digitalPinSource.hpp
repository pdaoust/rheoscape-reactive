#ifndef RHEOSCAPE_DIGITAL_PIN_SOURCE
#define RHEOSCAPE_DIGITAL_PIN_SOURCE

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

source_fn<bool> digitalPinSource(int pin, uint8_t pinModeFlag) {
  return [pin, pinModeFlag](push_fn<bool> push) {
    return [pin, pinModeFlag, push]() {
      pinMode(pin, INPUT | pinModeFlag);
      push((bool)digitalRead(pin));
    };
  };
}

#endif