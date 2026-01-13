#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sources::arduino {

  struct digital_pin_pull_handler {
    int pin;
    uint8_t pin_mode_flag;
    push_fn<bool> push;

    RHEO_NOINLINE void operator()() const {
      pinMode(pin, INPUT | pin_mode_flag);
      push(static_cast<bool>(digitalRead(pin)));
    }
  };

  struct digital_pin_source_binder {
    int pin;
    uint8_t pin_mode_flag;

    RHEO_NOINLINE pull_fn operator()(push_fn<bool> push) const {
      return digital_pin_pull_handler{pin, pin_mode_flag, std::move(push)};
    }
  };

  source_fn<bool> digital_pin_source(int pin, uint8_t pin_mode_flag) {
    return digital_pin_source_binder{pin, pin_mode_flag};
  }

}