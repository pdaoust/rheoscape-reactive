#pragma once

#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sinks::arduino {

  namespace detail {

    struct analog_pin_push_handler {
      int pin;

      RHEO_CALLABLE void operator()(int value) const {
        analogWrite(pin, value);
      }
    };

    struct analog_pin_sink_binder {
      int pin;

      template <typename SourceFn>
        requires concepts::SourceOf<SourceFn, int>
      RHEO_CALLABLE auto operator()(SourceFn source) const {
        return source(analog_pin_push_handler{pin});
      }
    };

  } // namespace detail

  auto analog_pin_sink(int pin, uint8_t resolution_bits = 10, uint32_t frequency = 0) {
    pinMode(pin, OUTPUT);
    analogWriteResolution(resolution_bits);
    if (frequency > 0) {
      analogWriteFreq(frequency);
    }
    return detail::analog_pin_sink_binder{pin};
  }

}
