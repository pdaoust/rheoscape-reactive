#pragma once

#include <types/core_types.hpp>
#include <Arduino.h>

namespace rheoscape::sinks::arduino {

  namespace detail {

    struct digital_pin_push_handler {
      int pin;

      RHEOSCAPE_CALLABLE void operator()(bool value) const {
        digitalWrite(pin, value);
      }
    };

    struct digital_pin_sink_binder {
      int pin;

      template <typename SourceFn>
        requires concepts::SourceOf<SourceFn, bool>
      RHEOSCAPE_CALLABLE auto operator()(SourceFn source) const {
        return source(digital_pin_push_handler{pin});
      }
    };

  } // namespace detail

  auto digital_pin_sink(int pin) {
    pinMode(pin, OUTPUT);
    return detail::digital_pin_sink_binder{pin};
  }

}
