#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sources::arduino {

  source_fn<int> analog_pin_source(int pin, uint8_t resolution_bits = 10) {
    pinMode(pin, INPUT);
    analogReadResolution(resolution_bits);

    struct SourceBinder {
      int pin;

      RHEO_CALLABLE pull_fn operator()(push_fn<int> push) const {
        struct PullHandler {
          int pin;
          push_fn<int> push;

          RHEO_CALLABLE void operator()() const {
            push(analogRead(pin));
          }
        };

        return PullHandler{pin, std::move(push)};
      }
    };

    return SourceBinder{pin};
  }

}
