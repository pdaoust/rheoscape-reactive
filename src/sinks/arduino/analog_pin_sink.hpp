#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sinks::arduino {

  struct analog_pin_sink_push_handler {
    int pin;
    uint8_t resolution_bits;
    uint32_t frequency;

    RHEO_NOINLINE void operator()(int value) const {
      pinMode(pin, OUTPUT);
      analogWriteResolution(resolution_bits);
      if (frequency > 0) {
        analogWriteFreq(frequency);
      }
      analogWrite(pin, value);  // Fixed: was digitalWrite
    }
  };

  struct analog_pin_sink_binder {
    int pin;
    uint8_t resolution_bits;
    uint32_t frequency;

    RHEO_NOINLINE pull_fn operator()(source_fn<int> source) const {
      return source(analog_pin_sink_push_handler{pin, resolution_bits, frequency});
    }
  };

  pullable_sink_fn<int> analog_pin_sink(int pin, uint8_t resolution_bits = 10, uint32_t frequency = 0) {
    return analog_pin_sink_binder{pin, resolution_bits, frequency};
  }

}