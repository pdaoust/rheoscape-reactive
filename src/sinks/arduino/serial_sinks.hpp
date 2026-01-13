#pragma once

#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo::sinks::arduino {

  struct serial_string_push_handler {
    RHEO_NOINLINE void operator()(std::string value) const {
      Serial.print(value.c_str());
    }
  };

  struct serial_string_sink_binder {
    RHEO_NOINLINE pull_fn operator()(source_fn<std::string> source) const {
      return source(serial_string_push_handler{});
    }
  };

  pullable_sink_fn<std::string> serial_string_sink() {
    return serial_string_sink_binder{};
  }

  struct serial_string_line_push_handler {
    RHEO_NOINLINE void operator()(std::string value) const {
      Serial.println(value.c_str());
    }
  };

  struct serial_string_line_sink_binder {
    RHEO_NOINLINE pull_fn operator()(source_fn<std::string> source) const {
      return source(serial_string_line_push_handler{});
    }
  };

  pullable_sink_fn<std::string> serial_string_line_sink() {
    return serial_string_line_sink_binder{};
  }

}