#pragma once

#include <types/core_types.hpp>
#include <Arduino.h>

namespace rheoscape::sinks::arduino {

  namespace detail {

    struct serial_string_push_handler {
      RHEOSCAPE_CALLABLE void operator()(std::string value) const {
        Serial.print(value.c_str());
      }
    };

    struct serial_string_sink_binder {
      template <typename SourceFn>
        requires concepts::SourceOf<SourceFn, std::string>
      RHEOSCAPE_CALLABLE auto operator()(SourceFn source) const {
        return source(serial_string_push_handler{});
      }
    };

    struct serial_string_line_push_handler {
      RHEOSCAPE_CALLABLE void operator()(std::string value) const {
        Serial.println(value.c_str());
      }
    };

    struct serial_string_line_sink_binder {
      template <typename SourceFn>
        requires concepts::SourceOf<SourceFn, std::string>
      RHEOSCAPE_CALLABLE auto operator()(SourceFn source) const {
        return source(serial_string_line_push_handler{});
      }
    };

  } // namespace detail

  auto serial_string_sink() {
    return detail::serial_string_sink_binder{};
  }

  auto serial_string_line_sink() {
    return detail::serial_string_line_sink_binder{};
  }

}
