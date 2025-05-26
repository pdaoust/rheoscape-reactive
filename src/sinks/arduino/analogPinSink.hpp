#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo {

  pullable_sink_fn<int> analogPinSink(int pin, uint8_t resolutionBits = 10) {
    return [pin, resolutionBits](source_fn<int> source) {
      return source(
        [pin, resolutionBits](int value) {
          pinMode(pin, OUTPUT);
          analogWriteResolution(resolutionBits);
          digitalWrite(pin, value);
        },
        [](){}
      )
    };
  }

}