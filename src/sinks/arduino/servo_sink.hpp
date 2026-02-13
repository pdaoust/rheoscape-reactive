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

  pullable_sink_fn<au::QuantityPoint<au::Degrees, uint8_t>> servo_sink(int pin) {
    struct SinkBinder {
      int pin;

      RHEO_CALLABLE pull_fn operator()(source_fn<au::QuantityPoint<au::Degrees, uint8_t>> source) const {
        auto servo = std::make_shared<Servo>();
        servo->attach(pin);

        struct PushHandler {
          std::shared_ptr<Servo> servo;

          RHEO_CALLABLE void operator()(au::QuantityPoint<au::Degrees, uint8_t> value) const {
            servo->write(static_cast<int>(value.in(au::Degrees{})));
          }
        };

        return source(PushHandler{std::move(servo)});
      }
    };

    return SinkBinder{pin};
  }

}
