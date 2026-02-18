#pragma once

#include <types/core_types.hpp>
#include <Arduino.h>

namespace rheoscape::sources::arduino {

  namespace detail {

    template <typename PushFn>
    struct analog_pin_pull_handler {
      int pin;
      PushFn push;

      RHEOSCAPE_CALLABLE void operator()() const {
        push(analogRead(pin));
      }
    };

    struct analog_pin_source_binder {
      using value_type = int;
      int pin;

      template <typename PushFn>
        requires concepts::Visitor<PushFn, int>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        return analog_pin_pull_handler<PushFn>{pin, std::move(push)};
      }
    };

  } // namespace detail

  auto analog_pin_source(int pin, uint8_t resolution_bits = 10) {
    pinMode(pin, INPUT);
    analogReadResolution(resolution_bits);
    return detail::analog_pin_source_binder{pin};
  }

}
