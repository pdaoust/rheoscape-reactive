#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sources::arduino {

  struct analog_pin_pull_handler {
    int pin;
    uint8_t resolution_bits;
    push_fn<int> push;

    RHEO_NOINLINE void operator()() const {
      analogReadResolution(resolution_bits);
      push(analogRead(pin));
    }
  };

  struct analog_pin_source_binder {
    int pin;
    uint8_t resolution_bits;

    RHEO_NOINLINE pull_fn operator()(push_fn<int> push) const {
      return analog_pin_pull_handler{pin, resolution_bits, std::move(push)};
    }
  };

  source_fn<int> analog_pin_source(int pin, uint8_t resolution_bits = 10) {
    return analog_pin_source_binder{pin, resolution_bits};
  }

}