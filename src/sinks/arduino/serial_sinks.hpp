#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sinks::arduino {

  pullable_sink_fn<std::string> serial_string_sink() {
    struct SinkBinder {
      RHEO_CALLABLE pull_fn operator()(source_fn<std::string> source) const {
        struct PushHandler {
          RHEO_CALLABLE void operator()(std::string value) const {
            Serial.print(value.c_str());
          }
        };

        return source(PushHandler{});
      }
    };

    return SinkBinder{};
  }

  pullable_sink_fn<std::string> serial_string_line_sink() {
    struct SinkBinder {
      RHEO_CALLABLE pull_fn operator()(source_fn<std::string> source) const {
        struct PushHandler {
          RHEO_CALLABLE void operator()(std::string value) const {
            Serial.println(value.c_str());
          }
        };

        return source(PushHandler{});
      }
    };

    return SinkBinder{};
  }

}
