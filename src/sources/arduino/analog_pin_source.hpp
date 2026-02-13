#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sources::arduino {

  struct analog_pin_pull_handler {
    int pin;
    push_fn<int> push;

    RHEO_CALLABLE void operator()() const {
      push(analogRead(pin));
    }
  };

  struct analog_pin_source_binder {
    int pin;

    RHEO_CALLABLE pull_fn operator()(push_fn<int> push) const {
      return analog_pin_pull_handler{pin, std::move(push)};
    }
  };

  source_fn<int> analog_pin_source(int pin, uint8_t resolution_bits = 10) {
    pinMode(pin, INPUT);
    analogReadResolution(resolution_bits);
    return analog_pin_source_binder{pin};
  }

}