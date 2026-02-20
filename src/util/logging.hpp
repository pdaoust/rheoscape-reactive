#pragma once

#include <functional>
#include <optional>
#include <string>

namespace rheoscape::logging {

  const uint8_t LOG_LEVEL_SILENT = 0;
  const uint8_t LOG_LEVEL_ERROR = 1;
  const uint8_t LOG_LEVEL_WARN = 2;
  const uint8_t LOG_LEVEL_INFO = 3;
  const uint8_t LOG_LEVEL_DEBUG = 4;
  const uint8_t LOG_LEVEL_TRACE = 5;
  #define LOG_LEVEL_LABEL(l) (l == rheoscape::logging::LOG_LEVEL_ERROR ? "ERROR" : l == rheoscape::logging::LOG_LEVEL_WARN ? "WARN" : l == rheoscape::logging::LOG_LEVEL_INFO ? "INFO" : l == rheoscape::logging::LOG_LEVEL_DEBUG ? "DEBUG" : l == rheoscape::logging::LOG_LEVEL_TRACE ? "TRACE" : "")

  namespace {
    static std::vector<std::function<void(uint8_t, std::optional<std::string>, std::string)>> _log_subscribers;
  }

  void register_subscriber(std::function<void(uint8_t, std::optional<std::string>, std::string)> subscriber) {
    _log_subscribers.push_back(subscriber);
  }

  void log(uint8_t level, std::optional<std::string> topic, std::string message) {
    for (auto subscriber: _log_subscribers) {
      subscriber(level, topic, message);
    }
  }

  #define RHEOSCAPE_LOGGING_LOG_LEVEL(label, level) void label(std::optional<std::string> topic, std::string message) { log(level, topic, message); }

  RHEOSCAPE_LOGGING_LOG_LEVEL(error, rheoscape::logging::LOG_LEVEL_ERROR)
  RHEOSCAPE_LOGGING_LOG_LEVEL(warn, rheoscape::logging::LOG_LEVEL_WARN)
  RHEOSCAPE_LOGGING_LOG_LEVEL(info, rheoscape::logging::LOG_LEVEL_INFO)
  RHEOSCAPE_LOGGING_LOG_LEVEL(debug, rheoscape::logging::LOG_LEVEL_DEBUG)
  RHEOSCAPE_LOGGING_LOG_LEVEL(trace, rheoscape::logging::LOG_LEVEL_TRACE)

}