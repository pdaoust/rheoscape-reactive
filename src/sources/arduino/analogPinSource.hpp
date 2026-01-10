#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sources::arduino {

  struct analog_pin_pull_handler {
    int pin;
    uint8_t resolutionBits;
    push_fn<int> push;

    RHEO_NOINLINE void operator()() const {
      analogReadResolution(resolutionBits);
      push(analogRead(pin));
    }
  };

  struct analog_pin_source_binder {
    int pin;
    uint8_t resolutionBits;

    RHEO_NOINLINE pull_fn operator()(push_fn<int> push) const {
      return analog_pin_pull_handler{pin, resolutionBits, std::move(push)};
    }
  };

  source_fn<int> analogPinSource(int pin, uint8_t resolutionBits = 10) {
    return analog_pin_source_binder{pin, resolutionBits};
  }

}