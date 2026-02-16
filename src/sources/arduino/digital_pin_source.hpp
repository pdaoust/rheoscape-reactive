#pragma once

#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sources::arduino {

  namespace detail {

    template <typename PushFn>
    struct digital_pin_pull_handler {
      int pin;
      PushFn push;

      RHEO_CALLABLE void operator()() const {
        push(static_cast<bool>(digitalRead(pin)));
      }
    };

    struct digital_pin_source_binder {
      using value_type = bool;
      int pin;

      template <typename PushFn>
        requires concepts::Visitor<PushFn, bool>
      RHEO_CALLABLE auto operator()(PushFn push) const {
        return digital_pin_pull_handler<PushFn>{pin, std::move(push)};
      }
    };

  } // namespace detail

  auto digital_pin_source(int pin, uint8_t pin_mode_flag) {
    pinMode(pin, INPUT | pin_mode_flag);
    return detail::digital_pin_source_binder{pin};
  }

}
