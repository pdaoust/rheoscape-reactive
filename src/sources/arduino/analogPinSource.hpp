#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo {

  source_fn<float> analogPinSource(int pin, uint8_t resolutionBits = 10) {
    return [pin, resolutionBits](push_fn<float> push, end_fn _) {
      return [pin, resolutionBits, push]() {
        analogReadResolution(resolutionBits);
        push(analogRead(pin));
      };
    };
  }

}