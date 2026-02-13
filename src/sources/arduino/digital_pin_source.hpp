#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sources::arduino {

  source_fn<bool> digital_pin_source(int pin, uint8_t pin_mode_flag) {
    pinMode(pin, INPUT | pin_mode_flag);

    struct SourceBinder {
      int pin;

      RHEO_CALLABLE pull_fn operator()(push_fn<bool> push) const {
        struct PullHandler {
          int pin;
          push_fn<bool> push;

          RHEO_CALLABLE void operator()() const {
            push(static_cast<bool>(digitalRead(pin)));
          }
        };

        return PullHandler{pin, std::move(push)};
      }
    };

    return SourceBinder{pin};
  }

}
