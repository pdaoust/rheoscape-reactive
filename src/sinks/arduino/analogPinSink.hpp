#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sinks::arduino {

  struct analog_pin_sink_push_handler {
    int pin;
    uint8_t resolutionBits;

    RHEO_NOINLINE void operator()(int value) const {
      pinMode(pin, OUTPUT);
      analogWriteResolution(resolutionBits);
      analogWrite(pin, value);  // Fixed: was digitalWrite
    }
  };

  struct analog_pin_sink_binder {
    int pin;
    uint8_t resolutionBits;

    RHEO_NOINLINE pull_fn operator()(source_fn<int> source) const {
      return source(analog_pin_sink_push_handler{pin, resolutionBits});
    }
  };

  pullable_sink_fn<int> analogPinSink(int pin, uint8_t resolutionBits = 10) {
    return analog_pin_sink_binder{pin, resolutionBits};
  }

}