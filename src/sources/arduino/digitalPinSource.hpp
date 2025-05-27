#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sources::arduino {

  source_fn<bool> digitalPinSource(int pin, uint8_t pinModeFlag) {
    return [pin, pinModeFlag](push_fn<bool> push, end_fn _) {
      return [pin, pinModeFlag, push]() {
        pinMode(pin, INPUT | pinModeFlag);
        push((bool)digitalRead(pin));
      };
    };
  }

}