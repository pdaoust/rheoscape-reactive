#pragma once

#include <memory>
#include <types/core_types.hpp>
#include <types/au_all_units_noio.hpp>
#include <Arduino.h>
#if defined(ARDUINO_ARCH_ESP32)
#include <ESP32Servo.h>
#else
#include <Servo.h>
#endif

namespace rheoscape::sinks::arduino {

  namespace detail {

    struct servo_push_handler {
      std::shared_ptr<Servo> servo;

      RHEOSCAPE_CALLABLE void operator()(au::QuantityPoint<au::Degrees, uint8_t> value) const {
        servo->write(static_cast<int>(value.in(au::Degrees{})));
      }
    };

    struct servo_sink_binder {
      int pin;

      template <typename SourceFn>
        requires concepts::SourceOf<SourceFn, au::QuantityPoint<au::Degrees, uint8_t>>
      RHEOSCAPE_CALLABLE auto operator()(SourceFn source) const {
        auto servo = std::make_shared<Servo>();
        servo->attach(pin);
        return source(servo_push_handler{std::move(servo)});
      }
    };

  } // namespace detail

  auto servo_sink(int pin) {
    return detail::servo_sink_binder{pin};
  }

}
