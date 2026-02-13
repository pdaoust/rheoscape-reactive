#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sinks::arduino {

  pullable_sink_fn<bool> digital_pin_sink(int pin) {
    pinMode(pin, OUTPUT);

    struct SinkBinder {
      int pin;

      RHEO_CALLABLE pull_fn operator()(source_fn<bool> source) const {
        struct PushHandler {
          int pin;

          RHEO_CALLABLE void operator()(bool value) const {
            digitalWrite(pin, value);
          }
        };

        return source(PushHandler{pin});
      }
    };

    return SinkBinder{pin};
  }

}
