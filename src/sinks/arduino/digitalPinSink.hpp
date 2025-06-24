#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sinks::arduino {

  pullable_sink_fn<bool> digitalPinSink(int pin) {
    return [pin](source_fn<bool> source) {
      return source([pin](bool value) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, value);
      });
    };
  }

}