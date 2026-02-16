#pragma once

#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sinks::arduino {

  namespace detail {

    struct digital_pin_push_handler {
      int pin;

      RHEO_CALLABLE void operator()(bool value) const {
        digitalWrite(pin, value);
      }
    };

    struct digital_pin_sink_binder {
      int pin;

      template <typename SourceFn>
        requires concepts::SourceOf<SourceFn, bool>
      RHEO_CALLABLE auto operator()(SourceFn source) const {
        return source(digital_pin_push_handler{pin});
      }
    };

  } // namespace detail

  auto digital_pin_sink(int pin) {
    pinMode(pin, OUTPUT);
    return detail::digital_pin_sink_binder{pin};
  }

}
