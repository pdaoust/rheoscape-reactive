#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sinks::arduino {

  pullable_sink_fn<int> analog_pin_sink(int pin, uint8_t resolution_bits = 10, uint32_t frequency = 0) {
    pinMode(pin, OUTPUT);
    analogWriteResolution(resolution_bits);
    if (frequency > 0) {
      analogWriteFreq(frequency);
    }

    struct SinkBinder {
      int pin;

      RHEO_CALLABLE pull_fn operator()(source_fn<int> source) const {
        struct PushHandler {
          int pin;

          RHEO_CALLABLE void operator()(int value) const {
            analogWrite(pin, value);
          }
        };

        return source(PushHandler{pin});
      }
    };

    return SinkBinder{pin};
  }

}
