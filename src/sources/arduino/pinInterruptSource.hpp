#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo {

  source_fn<bool> pinInterruptSource(int pin, uint8_t pinModeFlag) {
    return [pin, pinModeFlag](push_fn<bool> push, end_fn _) {
      attachInterrupt(
        digitalPinToInterrupt(pin),
        [pinModeFlag, push]() {
          if (pinModeFlag == LOW || pinModeFlag == FALLING) {
            push(false);
          } else if (pinModeFlag == HIGH || pinModeFlag == RISING) {
            push(true);
          } else if (pinModeFlag == CHANGE) {
            push(digitalRead(pin));
          }
        },
        pinModeFlag
      );
    };
  }

}