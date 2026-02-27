#pragma once

#include <functional>
#include <types/core_types.hpp>
#include <fmt/format.h>
#include <types/Fallible.hpp>
#include <util/logging.hpp>
#include <operators/tee.hpp>

namespace rheoscape::operators {

  namespace detail {
    template <typename FallibleT, typename MapFn>
    struct ErrorLoggerPushHandler {
      std::optional<std::string> topic;
      MapFn format_error;

      RHEOSCAPE_CALLABLE void operator()(FallibleT value) const {
        if (value.is_error()) {
          logging::error(topic, format_error(value.error()));
        }
      }
    };

    template <typename FallibleT, typename MapFn>
    struct ErrorLogger {
      std::optional<std::string> topic;
      MapFn format_error;

      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEOSCAPE_CALLABLE void operator()(SourceT source) const {
        source(ErrorLoggerPushHandler<FallibleT, MapFn>{topic, format_error});
      }
    };
  }

  template <typename SourceT, typename MapFn>
    requires concepts::Source<SourceT>
  auto log_errors(
    SourceT source,
    MapFn&& format_error,
    std::optional<std::string> topic = std::nullopt
  ) {
    using FallibleT = source_value_t<SourceT>;
    using MapFnDecayed = std::decay_t<MapFn>;

    return tee(
      std::move(source),
      detail::ErrorLogger<FallibleT, MapFnDecayed>{topic, std::forward<MapFn>(format_error)}
    );
  }

  // Overload with default formatter
  template <typename SourceT>
    requires concepts::Source<SourceT>
  auto log_errors(
    SourceT source,
    std::optional<std::string> topic = std::nullopt
  ) {
    using FallibleT = source_value_t<SourceT>;
    using TErr = typename FallibleT::error_type;

    struct DefaultFormatter {
      RHEOSCAPE_CALLABLE std::string operator()(TErr value) const {
        return fmt::format("{}", value);
      }
    };

    return log_errors(std::move(source), DefaultFormatter{}, topic);
  }

  namespace detail {
    template <typename T, typename TErr, typename MapFn>
    struct LogErrorsPipeFactory {
      MapFn format_error;
      std::optional<std::string> topic;

      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return log_errors(std::move(source), MapFn(format_error), topic);
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
