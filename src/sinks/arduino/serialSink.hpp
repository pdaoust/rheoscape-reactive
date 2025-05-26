#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo {
  pullable_sink_fn<std::string> serialSink() {
    return [](source_fn<std::string> source) {
      return source(
        [](std::string value) {
          Serial.print(value);
        },
        [](){}
      );
    };
  }
}