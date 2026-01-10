#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sources::arduino {

  struct digital_pin_pull_handler {
    int pin;
    uint8_t pinModeFlag;
    push_fn<bool> push;

    RHEO_NOINLINE void operator()() const {
      pinMode(pin, INPUT | pinModeFlag);
      push(static_cast<bool>(digitalRead(pin)));
    }
  };

  struct digital_pin_source_binder {
    int pin;
    uint8_t pinModeFlag;

    RHEO_NOINLINE pull_fn operator()(push_fn<bool> push) const {
      return digital_pin_pull_handler{pin, pinModeFlag, std::move(push)};
    }
  };

  source_fn<bool> digitalPinSource(int pin, uint8_t pinModeFlag) {
    return digital_pin_source_binder{pin, pinModeFlag};
  }

}