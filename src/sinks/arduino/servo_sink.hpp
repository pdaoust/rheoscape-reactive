#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/au_all_units_noio.hpp>
#include <Arduino.h>
#if defined(PLATFORM_ESP32)
#include <ESP32Servo.h>
#else
#include <Servo.h>
#endif


namespace rheo::sinks::arduino {

  struct servo_sink_push_handler {
    std::shared_ptr<Servo> servo;
    int pin;

    RHEO_CALLABLE void operator()(au::QuantityPoint<au::Degrees, uint8_t> value) const {
        servo->write(static_cast<int>(value.in(au::Degrees{})));
    }
  };

  struct servo_sink_binder {
    int pin;

    RHEO_CALLABLE pull_fn operator()(source_fn<au::QuantityPoint<au::Degrees, uint8_t>> source) const {
      auto servo = std::make_shared<Servo>();
      servo->attach(pin);
      return source(servo_sink_push_handler{std::move(servo), pin});
    }
  };

  pullable_sink_fn<au::QuantityPoint<au::Degrees, uint8_t>> servo_sink(int pin) {
    return servo_sink_binder{pin};
  }

}