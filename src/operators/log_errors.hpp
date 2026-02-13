#pragma once

#include <functional>
#include <core_types.hpp>
#include <fmt/format.h>
#include <Fallible.hpp>
#include <logging.hpp>
#include <operators/tee.hpp>

namespace rheo::operators {

  template <typename T, typename TErr, typename MapFn>
  source_fn<Fallible<T, TErr>> log_errors(
    source_fn<Fallible<T, TErr>> source,
    MapFn&& format_error,
    std::optional<std::string> topic = std::nullopt
  ) {
    using MapFnDecayed = std::decay_t<MapFn>;

    struct ErrorLogger {
      std::optional<std::string> topic;
      MapFnDecayed format_error;

      RHEO_CALLABLE void operator()(source_fn<Fallible<T, TErr>> source) const {
        struct PushHandler {
          std::optional<std::string> topic;
          MapFnDecayed format_error;

          RHEO_CALLABLE void operator()(Fallible<T, TErr> value) const {
            if (value.is_error()) {
              logging::error(topic, format_error(value.error()));
            }
          }
        };

        source(PushHandler{topic, format_error});
      }
    };

    return tee(source, ErrorLogger{topic, std::forward<MapFn>(format_error)});
  }

  // Overload with default formatter
  template <typename T, typename TErr>
  source_fn<Fallible<T, TErr>> log_errors(
    source_fn<Fallible<T, TErr>> source,
    std::optional<std::string> topic = std::nullopt
  ) {
    struct DefaultFormatter {
      RHEO_CALLABLE std::string operator()(TErr value) const {
        return fmt::format("{}", value);
      }
    };

    return log_errors(source, DefaultFormatter{}, topic);
  }

  namespace detail {
    template <typename T, typename TErr, typename MapFn>
    struct LogErrorsPipeFactory {
      MapFn format_error;
      std::optional<std::string> topic;

      RHEO_CALLABLE source_fn<Fallible<T, TErr>> operator()(source_fn<Fallible<T, TErr>> source) const {
        return log_errors(source, MapFn(format_error), topic);
      }
    };
  }

  template <typename T, typename TErr, typename MapFn>
  auto log_errors(
    MapFn&& format_error,
    std::optional<std::string> topic = std::nullopt
  ) {
    return detail::LogErrorsPipeFactory<T, TErr, std::decay_t<MapFn>>{
      std::forward<MapFn>(format_error), topic
    };
  }

}
