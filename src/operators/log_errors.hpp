#pragma once

#include <functional>
#include <core_types.hpp>
#include <fmt/format.h>
#include <Fallible.hpp>
#include <logging.hpp>
#include <operators/tap.hpp>

namespace rheo::operators {

  template <typename T, typename TErr, typename MapFn>
  source_fn<Fallible<T, TErr>> log_errors(
    source_fn<Fallible<T, TErr>> source,
    MapFn&& format_error = [](TErr value) { return fmt::format("{}", value); },
    std::optional<std::string> topic = std::nullopt
  ) {
    return tap(source, [topic, format_error = std::forward<MapFn>(format_error)](source_fn<Fallible<T, TErr>> source) {
      source([topic, format_error](Fallible<T, TErr> value) {
        if (value.is_error()) {
          logging::error(topic, format_error(value.error()));
        }
      });
    });
  }

  template <typename T, typename TErr, typename MapFn>
  pipe_fn<Fallible<T, TErr>, Fallible<T, TErr>> log_errors(
    MapFn&& format_error = [](TErr value) { return fmt::format("{}", value); },
    std::optional<std::string> topic = std::nullopt
  ) {
    return [format_error, topic](source_fn<Fallible<T, TErr>> source) {
      return log_errors(source, format_error, topic);
    };
  }

}