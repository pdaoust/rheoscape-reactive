#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sources::arduino {

  source_fn<int> analogPinSource(int pin, uint8_t resolutionBits = 10) {
    return [pin, resolutionBits](push_fn<float>&& push) {
      return [pin, resolutionBits, push = std::forward<push_fn<float>>(push)]() {
        analogReadResolution(resolutionBits);
        push(analogRead(pin));
      };
    };
  }

}