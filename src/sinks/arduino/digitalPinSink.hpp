#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sinks::arduino {

  struct digital_pin_sink_push_handler {
    int pin;

    RHEO_NOINLINE void operator()(bool value) const {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, value);
    }
  };

  struct digital_pin_sink_binder {
    int pin;

    RHEO_NOINLINE pull_fn operator()(source_fn<bool> source) const {
      return source(digital_pin_sink_push_handler{pin});
    }
  };

  pullable_sink_fn<bool> digitalPinSink(int pin) {
    return digital_pin_sink_binder{pin};
  }

}