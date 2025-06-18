#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sinks::arduino {

  pullable_sink_fn<std::string> serialStringSink() {
    return [](source_fn<std::string> source) {
      return source(
        [](std::string value) {
          Serial.print(value.c_str());
        },
        [](){}
      );
    };
  }

  pullable_sink_fn<std::string> serialStringLineSink() {
    return [](source_fn<std::string> source) {
      return source(
        [](std::string value) {
          Serial.println(value.c_str());
        },
        [](){}
      );
    };
  }

}